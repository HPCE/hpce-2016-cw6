#ifndef heat_hpp
#define heat_hpp

#include <vector>
#include <array>
#include <unordered_map>
#include <deque>
#include <cassert>
#include <climits>
#include <unistd.h>
#include <iostream>

#include "jpeg_helpers.hpp"

struct heat
{
    static const char *type_name()
    { return "heat"; }
    
    
    struct graph_type
    {
        std::string topology;   // "rect" | "hex" | "mesh"
        
        uint16_t width, height; // Dimensions in pixel-terms
        uint32_t maxTime;       // When to stop stepping
        uint32_t outputDelta;   // Delay between output slices
        
        // Used for rendering limits
        int32_t minHeat;
        int32_t maxHeat;
    };
    
    struct message_type
    {
        uint32_t time;
        int32_t heat;
    };

    struct channel_type
    {
        int32_t weight;
    };

    struct properties_type
    {
        unsigned id;                // Unique id within graph
        uint32_t neighbourCount;    // How many inputs are connected to this node?
        uint16_t x, y;              // Position within simulation space
        int32_t selfWeight;         // Weighting of self in relaxation kernel.
        int32_t initValue;          // Value at startup
        bool isDirichlet;           // Indicates cells that have a forcing condition
        bool isOutput;              // Use to filter a sub-set of nodes
    };
        
    struct device_type
    {    
        uint32_t time;
        int32_t heat;
        int32_t accNow;
        uint32_t seenNow;
        int32_t accNext;
        uint32_t seenNext;
    };
    
    /////////////////////////////////////////////////////////////
    // Things used used during actual execution of devices
    
    static int32_t mul_fix16(int32_t a, int32_t b)
    {
        // Do multiply, add 0.5, then round to integer
        return (a * b + 0x8000) >> 16; 
    }
    
    static void on_init(
        const graph_type *graph,
        const properties_type *properties,
        device_type *state
    ){
        state->time=0;
        if(properties->isDirichlet){
            state->heat=properties->initValue;
        }else{
            state->heat=0;
        }
        state->accNow = properties->initValue;
        state->seenNow = properties->neighbourCount;
        state->accNext = 0;
        state->seenNext = 0;
    }
    
    static bool ready_to_send(
        const graph_type *graph,
        const properties_type *properties,
        const device_type *state
    ){
        return (state->time < graph->maxTime)
            && (state->seenNow == properties->neighbourCount);
    }
    
    static void on_recv(
        const graph_type *graph,
        const channel_type *channel,
        const message_type *messageIn,
        const properties_type *properties,
        device_type *state
    ){  
        // How much does heat from this edge contribute?
        int32_t weightedHeat = mul_fix16(channel->weight, messageIn->heat); 
        
        // Accumulate for this time-step or the next
        if(messageIn->time == state->time){
            state->seenNow += 1;
            state->accNow  += weightedHeat;
        }else if(messageIn->time == state->time+1){
            state->seenNext += 1;
            state->accNext  += weightedHeat;
        }else{
            assert(0); // Should never happen
        }
    }

    static bool on_send(
        const graph_type *graph,
        message_type *messageOut,
        const properties_type *properties,
        device_type *state
    ){
        assert( ready_to_send(graph, properties, state) );
        
        // Update state
        state->time = state->time+1;
        if(properties->isDirichlet){
            state->heat = state->heat + (properties->initValue>>8);
            if(state->heat > graph->maxHeat){
                state->heat = graph->minHeat;
            }else if(state->heat < graph->minHeat){
                state->heat = graph->maxHeat;
            }
        }else{
            state->heat = state->accNow;            
        }
        
        int32_t weightedSelf = mul_fix16(properties->selfWeight, state->heat); 
        state->accNow = state->accNext + weightedSelf;
        state->seenNow = state->seenNext;
        state->accNext = 0;
        state->seenNext = 0;
        
        // Prepare message
        messageOut->time = state->time;
        messageOut->heat = state->heat;        
        
        // Should we output?
        return properties->isOutput && (0 == (state->time % graph->outputDelta));
    }
    
    
    /////////////////////////////////////////////////////////////
    // Used to manage extraction of data from the devices
    
    class SupervisorDevice
    {
    private:    
        typedef std::array<uint8_t,3> rgb_colour;
    
        std::vector<const properties_type *> m_indexToDevice;
        // Build a map from the device pointer to the index
        std::unordered_map<const properties_type *, unsigned> m_deviceToIndex;
        
        struct time_slice
        {
            unsigned time;
            unsigned seen;
            std::vector<int32_t> heat;
        };
                
        // Find the closest output to the given output point
        unsigned find_closest_device(uint16_t x, uint16_t y)
        {
            unsigned closestIndex=0;
            unsigned closestDistance=UINT_MAX   ;
            
            for(unsigned i=0; i<m_indexToDevice.size(); i++){
                int dx = int(x) - m_indexToDevice[i]->x;
                int dy = int(y) - m_indexToDevice[i]->y;
                unsigned d = dx*dx + dy*dy;
                
                if(d < closestDistance){
                    closestIndex = i;
                    closestDistance = d;
                }
            }
            
            return closestIndex;
        }
        
        rgb_colour choose_colour(const properties_type *device, int32_t heat)
        {
            rgb_colour colour;
            if( device->selfWeight == 0 ){
                colour[0]=0;
                colour[1]=0;
                colour[2]=0;
            }else{
                // Clamp to range [minHeat,maxHeat]
                heat = std::max(m_graph->minHeat, std::min(m_graph->maxHeat, heat));
                
                // Convert to range in 0..255
                float scale = 255.0f / (m_graph->maxHeat - m_graph->minHeat + 1);
                float val = (heat - m_graph->minHeat) * scale;
                //fprintf(stderr, "heat = %d, val=%f\n", heat, val);
                
                colour[0] = val;
                colour[1] = 0;
                colour[2] = 255-val;
            }
            return colour;
        }
        
        
        void renderSlice(
            const time_slice &slice
        ){
            unsigned scanWidth=3*m_graph->width;
            scanWidth= (scanWidth+3)&0xFFFFFFFCul; // pad up to a multiple of four
            
            std::vector<uint8_t> pixels(scanWidth*m_graph->height);
            
            for(unsigned y=0; y<m_graph->height; y++){
                for(unsigned x=0; x<m_graph->width; x++){
                    unsigned deviceIndex = find_closest_device(x,y);
                    
                    const properties_type *device = m_indexToDevice[deviceIndex];
                    int32_t heat = slice.heat[deviceIndex];
                    rgb_colour colour = choose_colour(device, heat);
                    
                    pixels[ y*scanWidth + x*3 + 0 ] = colour[0];
                    pixels[ y*scanWidth + x*3 + 1 ] = colour[1];
                    pixels[ y*scanWidth + x*3 + 2 ] = colour[2];
                    
                    //fprintf(stderr, " %4.2f (%x%x%x)", heat/65536.0, colour[0]>>4, colour[1]>>4,colour[2]>>4);
                }
                //fprintf(stderr, "\n");
            }
            //fprintf(stderr, "\n\n");

            
            write_JPEG_file (m_graph->width, m_graph->height, pixels, m_destFile, /*quality*/ 100);
        }
        
        const graph_type *m_graph;  
        FILE *m_destFile;
        
        // A deque (double-ended queue) allows us to push and pop from either end
        std::deque<time_slice> m_slices;
    public:
        SupervisorDevice(
            const graph_type *graph,
            FILE *destFile
        )
            : m_graph(graph)
            , m_destFile(destFile)
        {}
        
        void onAttachNode(const properties_type *device)
        {
            if(!device->isOutput)
                return; // Only track output devices
            unsigned index=m_indexToDevice.size();
            m_indexToDevice.push_back(device);
            m_deviceToIndex[device]=index;
        }
        
        void onDeviceOutput(
            const properties_type *device,
            const message_type *message
        ){
            //fprintf(stderr, "On device output %u, %u\n", device->id, message->time);
            
            unsigned devIndex=m_deviceToIndex.at(device);
            unsigned sliceIndex=0;
            
            //fprintf(stderr, "  Device index = %u\n", devIndex);
            
            // Search for the correct destination time slice
            while(sliceIndex < m_slices.size()){
                if(message->time == m_slices[sliceIndex].time)
                    break;
                sliceIndex++;
            }
            // ... or add a new slice if necessary
            if(sliceIndex == m_slices.size()){
                // Implication is that this is a new latest time
                assert( m_slices.size()==0 || (message->time > m_slices.back().time ) );
                
                time_slice newSlice;
                newSlice.time = message->time;
                newSlice.seen = 0;
                newSlice.heat.resize( m_indexToDevice.size() ); // Allocate one element per output pixel
                
                m_slices.push_back(newSlice);
            }
            
            
            // Insert the message into the time slice
            m_slices.at(sliceIndex).heat.at(devIndex) = message->heat;
            m_slices.at(sliceIndex).seen++; // And record that we have seen another output for this slice

            //fprintf(stderr, "  Slice index = %u, count = %u, target = %u\n", sliceIndex, m_slices.at(sliceIndex).seen, m_indexToDevice.size());
            
            // Finally... if we have got the entire next slice, then output it
            while(! m_slices.empty() && m_slices.front().seen == m_indexToDevice.size() ){
                //fprintf(stderr, "  Slice full!\n");
                // Render it and send it down the pipe
                renderSlice( m_slices.front() );
                // We are done with it, so remove it
                m_slices.pop_front();
            }
        }
    };
};


std::istream &operator>>(std::istream &src, heat::graph_type &g)
{ return src>>g.topology>>g.width>>g.height>>g.maxTime>>g.outputDelta>>g.minHeat>>g.maxHeat; }

std::ostream &operator<<(std::ostream &src, const heat::graph_type &g)
{ return src<<g.topology<<" "<<g.width<<" "<<g.height<<" "<<g.maxTime<<" "<<g.outputDelta<<" "<<g.minHeat<<" "<<g.maxHeat; }


std::istream &operator>>(std::istream &src, heat::channel_type &c)
{ return src>>c.weight; }

std::ostream &operator<<(std::ostream &dst, const heat::channel_type &c)
{ return dst<<c.weight; }


std::istream &operator>>(std::istream &src, heat::properties_type &p)
{ return src>>p.id>>p.neighbourCount>>p.x>>p.y>>p.selfWeight>>p.initValue>>p.isDirichlet>>p.isOutput; }
        
std::ostream &operator<<(std::ostream &src, const heat::properties_type &p)
{ return src<<p.id<<" "<<p.neighbourCount<<" "<<p.x<<" "<<p.y<<" "<<p.selfWeight<<" "<<p.initValue<<" "<<p.isDirichlet<<" "<<p.isOutput; }


#endif
    
