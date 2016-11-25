#include "util.hpp"

#include "simulator.hpp"
#include "graph_loader.hpp"


#include "graphs/heat.hpp"
#include "graphs/ring.hpp"



#include <cstdio>
#include <unistd.h>
#include <random>
#include <iostream> 
#include <fstream>
#include <cstring>

#include <sys/stat.h>
#include <fcntl.h>


template<class TGraph>
void simulate(int logLevel, unsigned &lineNumber, std::istream &src, std::ostream &stats, FILE *dst)
{
    typename TGraph::graph_type graph;
    unsigned numDevices, numChannels;
    
    graph_load_header<TGraph>(
        lineNumber, src,
        graph, numDevices, numChannels
    );
    
    if(logLevel > 0){
        fprintf(stderr, "Load: found graph of type '%s' with %u devices and %u channels\n", TGraph::type_name(), numDevices, numChannels);
    }
    
    Simulator<TGraph> sim(
        logLevel, stats, dst,
        graph, numDevices, numChannels
    );
    
    graph_load_body(
        lineNumber, src,
        numDevices, numChannels,    
        sim
    );
    
    sim.run();
}

void usage()
{
    fprintf(stderr, "usage: (srcFile|-) (statsFile|-) (outFile|-) [--log-level level]\n");
    fprintf(stderr, "  srcFile : Where to read the graph from (or - for stdin)\n");
    fprintf(stderr, "  statsFile : Where to write the statistics to (or - for stdout)\n");
    fprintf(stderr, "  outFile : Where to write the output to (or - for stdout)\n");
    fprintf(stderr, "    (you can't write both statsFile and outFile to stdout\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    try{        
        // Try to fix stdin/stdout on windows
        puzzler::WithBinaryIO binaryIO;
        
        std::istream *src=&std::cin;
        std::ifstream srcFile;
        
        std::ostream *stats=&std::cout;
        std::ofstream statsFile;
        
        FILE *dst=stdout;
        FILE *dstFile=0;
        
        int logLevel=1;
        
        //////////////////////////////////////////////////////////////////////
        // Argument parsing
        
        int ai=1;
        int pi=0;
        while(ai<argc){
            if(!strcmp(argv[ai], "--log-level")){
                if(argc-ai < 2){
                    fprintf(stderr, "Error: Missing parameter to --log-level\n");
                    exit(1);
                }
                logLevel = atoi(argv[ai+1]);
                ai+=2;
                fprintf(stderr, "Set log-level to %d\n", logLevel);
            }else if(pi==0){
                fprintf(stderr, "Setting srcFile to '%s'\n", argv[ai]);
                if(strcmp(argv[ai], "-")){
                    srcFile.open(argv[ai], std::ios_base::in);
                    if(!srcFile.is_open()){
                        fprintf(stderr, "Error: Couldn't open source file.\n");
                        exit(1);
                    }
                    src=&srcFile;
                }
                ai++;
                pi++;
            }else if(pi==1){
                fprintf(stderr, "Setting statsFile to '%s'\n", argv[ai]);
                if(strcmp(argv[ai], "-")){
                    statsFile.open(argv[ai], std::ios_base::out|std::ios_base::trunc);
                    if(!statsFile.is_open()){
                        fprintf(stderr, "Error: Couldn't open stats file.\n");
                        exit(1);
                    }
                    stats=&statsFile;    
                }
                ai++;
                pi++;
            }else if(pi==2){
                fprintf(stderr, "Setting dstFile to '%s'\n", argv[ai]);
                if(strcmp(argv[ai], "-")){
                    dstFile=fopen(argv[ai], "wb+");
                    if(dstFile==0){
                        fprintf(stderr, "Error: Couldn't open dest file.\n");
                        exit(1);
                    }
                    dst=dstFile;
                }
                ai++;
                pi++;
            }else{
                fprintf(stderr, "Error: Unknown argument '%s'\n", argv[ai]);
                exit(1);
            }
        }
        
        if( (dst==stdout) && (stats==&std::cout) ){
            fprintf(stderr, "Error: Can't send both stats and output to stdout (send one to /dev/null ?)\n");
            exit(1);
        }
        
        
        ///////////////////////////////////////////////////
        // Parsing and execution
        
        unsigned lineNumber=0;
        
        // Read the graph header, containing the type
        std::string type=graph_load_type(lineNumber, *src);
        
        if(type=="heat"){
            simulate<heat>(logLevel, lineNumber, *src, *stats, dst);
        }else if(type=="ring"){
            simulate<ring>(logLevel, lineNumber, *src, *stats, dst);
        }else{
            fprintf(stderr, "Error: Unknown graph type '%s'\n", type.c_str());
            exit(1);
        }
        
        
    }catch(std::exception &e){
        fprintf(stderr, "Exception: %s\n", e.what());
        exit(1);
    }
    
}
