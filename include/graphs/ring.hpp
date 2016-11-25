#ifndef ring_hpp
#define ring_hpp

#include <vector>
#include <array>
#include <unordered_map>
#include <deque>
#include <cassert>
#include <climits>
#include <unistd.h>
#include <iostream>

struct ring
{
    static const char *type_name()
    { return "ring"; }
    
    struct graph_type
    {};
    
    struct message_type
    {};

    struct channel_type
    {};

    struct properties_type
    {
        unsigned id;
        int initial;
    };
        
    struct device_type
    {    
        int current;
    };
    
    /////////////////////////////////////////////////////////////
    // Things used during actual execution of devices
    
    static void on_init(
        const graph_type *graph,
        const properties_type *properties,
        device_type *state
    ){
        state->current = 0;
    }
    
    static bool ready_to_send(
        const graph_type *graph,
        const properties_type *properties,
        const device_type *state
    ){
        if(properties->initial){
            return state->current==0;
        }else{
            return state->current==1;
        }
    }
    
    static void on_recv(
        const graph_type *graph,
        const channel_type */*channel*/,
        const message_type */*messageIn*/,
        const properties_type * /*properties*/,
        device_type *state
    ){  
        state->current = 1;
    }

    static bool on_send(
        const graph_type *graph,
        message_type */*messageOut*/,
        const properties_type *properties,
        device_type *state
    ){
        assert(properties->initial || state->current==1);
        state->current=0;
        return true;
    }
    
    
    /////////////////////////////////////////////////////////////
    // Used to manage extraction of data from the devices
    
    class SupervisorDevice
    {
    private:    
        
        const graph_type *m_graph;  
        FILE *m_destFile;
        
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
            // do nothing
        }
        
        void onDeviceOutput(
            const properties_type *device,
            const message_type *message
        ){
            fprintf(m_destFile, "Tick : %u\n", device->id);
        }
    };
};


std::istream &operator>>(std::istream &src, ring::graph_type &g)
{ return src; }

std::ostream &operator<<(std::ostream &src, const ring::graph_type &g)
{ return src; }


std::istream &operator>>(std::istream &src, ring::channel_type &c)
{ return src; }

std::ostream &operator<<(std::ostream &dst, const ring::channel_type &c)
{ return dst; }


std::istream &operator>>(std::istream &src, ring::properties_type &p)
{ return src>>p.id>>p.initial; }
        
std::ostream &operator<<(std::ostream &src, const ring::properties_type &p)
{ return src<<p.id<<" "<<p.initial; }


#endif
    
