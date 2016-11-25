#ifndef writer_hpp
#define writer_hpp

#include <cstdint>
#include <vector>
#include <deque>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>

std::string nextline(unsigned &lineNumber, std::istream &src)
{
    lineNumber++;
    std::string line;
    if(!std::getline(src, line)){          
        std::stringstream err;
        err<<"Couldn't read line number "<<lineNumber<<"."; 
        throw std::runtime_error(err.str());
    }
    return line;
};

void expect(unsigned &lineNumber, std::istream &src, const char *string)
{
    std::string token;
    std::stringstream(nextline(lineNumber,src)) >> token;
    
    if(token!=string){
        std::stringstream err;
        err<<"At line "<<lineNumber<<" : expecting '"<<string<<"', but got '"<<token<<"'";
        throw std::runtime_error(err.str());
    }
};

/* Generic part of loader that doesn't depend on graph type */
std::string graph_load_type(
    unsigned &lineNumber,
    std::istream &src
){
    expect(lineNumber, src, "POETSGraph");
    
    std::string type;
    std::stringstream(nextline(lineNumber, src)) >> type;
    
    return type;
}

template<class TGraph>
void graph_load_header(
    unsigned &lineNumber,
    std::istream &src,
    typename TGraph::graph_type &graph,
    unsigned &deviceCount,
    unsigned &channelCount
){    
    std::stringstream err;
    
    expect(lineNumber, src, "BeginHeader");
    
    expect(lineNumber, src, TGraph::type_name());
    
    if(! (std::stringstream(nextline(lineNumber,src)) >> deviceCount >> channelCount) ){
        err<<"At line "<<lineNumber<<" : Couldn't read device and channel count";
        throw std::runtime_error(err.str());
    }
 
    if(! (std::stringstream(nextline(lineNumber,src)) >> graph ) ){
        err<<"At line "<<lineNumber<<" : Couldn't read graph properties";
        throw std::runtime_error(err.str());
    }
       
    expect(lineNumber, src, "EndHeader");
    
}

template<class TGraphBuilder>
void graph_load_body(
    unsigned &lineNumber,
    std::istream &src,
    unsigned numDevices, unsigned numChannels,
    TGraphBuilder &dst
){    
    typedef typename TGraphBuilder::properties_type properties_type;
    typedef typename TGraphBuilder::channel_type channel_type;
    
    std::stringstream err;
    
    expect(lineNumber, src, "BeginNodes");
    
    std::vector<unsigned> devices;
    for(unsigned i=0; i<numDevices; i++){
        properties_type properties;
        if(! ( std::stringstream(nextline(lineNumber, src)) >> properties ) ){
            err<<"At line "<<lineNumber<<" : Couldn't read node "<<i;
            throw std::runtime_error(err.str());
        }
        unsigned deviceIndex = dst.addDevice(properties);
        devices.push_back(deviceIndex);
    }
    expect(lineNumber, src, "EndNodes");
    
    expect(lineNumber, src, "BeginEdges");
    for(unsigned i=0; i<numChannels; i++){
        unsigned dstIndex, srcIndex;
        channel_type channel;
        unsigned delay;
        
        if(! ( std::stringstream(nextline(lineNumber, src)) >> dstIndex >> srcIndex >> delay >> channel ) ){
            err<<"At line "<<lineNumber<<" : Couldn't read node "<<i;
            throw std::runtime_error(err.str());
        }
        
        dst.addChannel(dstIndex, srcIndex, delay, channel);
        
    }
    expect(lineNumber, src, "EndEdges");
}

#endif
