#include "graph_builder.hpp"
#include "graphs/heat.hpp"

#include <cstdio>
#include <unistd.h>
#include <random>
#include <iostream> 

#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    try{
        
        unsigned w=65, h=65;
        unsigned maxTime=64;
        unsigned outputDeltaSpace=2;
        unsigned outputDeltaTime=8;
        int minHeat=-30000, maxHeat=+30000;
        
        if(argc>1){
            w=atoi(argv[1]);
            h=w;
        }
        if(argc>2){
            maxTime=atoi(argv[2]);
        }
        if(argc>3){
            outputDeltaTime=atoi(argv[3]);
        }
        if(argc>4){
            outputDeltaSpace=atoi(argv[4]);
        }
        
        w= std::max(1u, (w/outputDeltaSpace))*outputDeltaSpace+1;
        h= std::max(1u, (h/outputDeltaSpace))*outputDeltaSpace+1;
        
        unsigned outputScale=std::max(1u, 256 / w);
        unsigned pixelWidth=(w-1)*outputScale+1;
        unsigned pixelHeight=(h-1)*outputScale+1;
        pixelWidth = ((pixelWidth+3)/4)*4;
        pixelHeight = ((pixelHeight+3)/4)*4;
        
        std::mt19937 urng;
        
        
        heat::graph_type graph{
            "rect",
            uint16_t(pixelWidth), uint16_t(pixelHeight),
            maxTime,
            outputDeltaTime,
            minHeat,
            maxHeat
        };
        
        GraphBuilder<heat> sim(graph);
        
        std::unordered_map<unsigned,unsigned> idToNodeIndex;
        
        float selfWeight = 0.5f;
        float otherWeight = (1-selfWeight)/4;
        
        for(unsigned y=0; y<h; y++){
            for(unsigned x=0; x<w; x++){
                bool edgeY = (y==0) || (y==h-1);
                bool edgeX = (x==0) || (x==w-1);
                unsigned neighbourCount = 4 - (edgeX?1:0) - (edgeY?1:0);
                
                uint32_t u=urng();
                heat::properties_type device{
                    y *  w + x,
                    neighbourCount,
                    uint16_t(x*outputScale), uint16_t(y*outputScale),
                    int32_t(selfWeight*65536),
                        edgeY ? int( ( sin( 3*( x/float(w) + y/float(w)) )) * 30000 ) :
                        edgeX ? ( (x==0) ? minHeat/2 : maxHeat/2 ) : 
                                ( int(u % (maxHeat-minHeat+1)) + minHeat),
                    edgeX || edgeY,
                    (0==x%outputDeltaSpace) && (0==y%outputDeltaSpace)
                };
                
                unsigned nodeIndex=sim.addDevice(device);
                idToNodeIndex[device.id] = nodeIndex;
                
                //fprintf(stderr, " (%u,%u) -> %u\n", x,y,nodeIndex);
            }
        }
        
        auto addChannel = [&](unsigned srcX, unsigned srcY, unsigned dstX, unsigned dstY, int32_t weight)
        {
            unsigned srcIdx=srcY*w+srcX;
            unsigned dstIdx=dstY*w+dstX;
            
            // Exponentially distributed delays
            unsigned u=urng() & 0xFFFF;
            unsigned delay=0;
            while(u&1){
                delay++;
                u=u>>1;
            }
            
            sim.addChannel(
                idToNodeIndex[srcIdx],
                idToNodeIndex[dstIdx],
                delay,
                heat::channel_type{
                    weight
                }
            );
        };

        for(unsigned y=0; y<h; y++){
            for(unsigned x=0; x<w; x++){
                if(x>0){
                    addChannel(x-1, y, x, y, otherWeight*65536); 
                }
                if(x+1<w){
                    addChannel(x+1, y, x, y, otherWeight*65536); 
                }
                if(y>0){
                    addChannel(x, y-1, x, y, otherWeight*65536); 
                }
                if(y+1<h){
                    addChannel(x, y+1, x, y, otherWeight*65536);
                }
            }
        }        
        
        sim.write(std::cout);
    }catch(...){
        std::cerr<<"Caught exception\n";
        exit(1);
        
    }
    
}
