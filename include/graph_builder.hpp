#ifndef writer_hpp
#define writer_hpp

#include <cstdint>
#include <vector>
#include <deque>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>

template<class TGraph>
class GraphBuilder
{
private:
    typedef typename TGraph::graph_type graph_type;
    typedef typename TGraph::properties_type properties_type;
    typedef typename TGraph::channel_type channel_type;
    
    struct node
    {
        properties_type properties;
    };
    
    struct edge
    {
        unsigned dst;
        unsigned src;
        unsigned delay;
        channel_type channel;
    };
    
    
    graph_type m_graph;
    
    std::deque<node> m_nodes;
    std::deque<edge> m_edges;
   
public:
    GraphBuilder(
        const graph_type &graph
    )
    : m_graph(graph)
    {}
    
    unsigned addDevice(
        const properties_type &device
    ){
        unsigned index=m_nodes.size();
        node n;
        n.properties=device;
        m_nodes.push_back(n);
        return index;
    }
    
    void addChannel(
        unsigned srcIndex,
        unsigned dstIndex,
        unsigned delay,
        const channel_type &channel
    ){
        edge e;
        e.src = srcIndex;
        e.dst = dstIndex;
        e.delay = delay;
        e.channel = channel;
        m_edges.push_back(e);
    }
    
    void write(std::ostream &dst) const
    {
        dst<<"POETSGraph"<<std::endl;
        dst<<TGraph::type_name()<<std::endl;
        
        dst<<"BeginHeader"<<std::endl;
        dst<<TGraph::type_name()<<std::endl;    // Write it again, for error checking
        dst<<m_nodes.size()<<" "<<m_edges.size()<<std::endl;
        dst<<m_graph<<std::endl;
        dst<<"EndHeader"<<std::endl;
        
        dst<<"BeginNodes"<<std::endl;
        for(unsigned i=0; i<m_nodes.size(); i++){
            dst<<m_nodes[i].properties<<std::endl;
        }
        dst<<"EndNodes"<<std::endl;
        
        dst<<"BeginEdges"<<std::endl;
        for(unsigned i=0; i<m_edges.size(); i++){
            dst<<m_edges[i].dst<<" "<<m_edges[i].src<<" "<<m_edges[i].delay<<" "<<m_edges[i].channel<<std::endl;
        }
        dst<<"EndEdges"<<std::endl;
    }
};

#endif
