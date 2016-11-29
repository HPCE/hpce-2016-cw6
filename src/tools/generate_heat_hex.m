% Minimal example of generating a hex graph. You can run it using
% octave (apt get install octave!) or matlab.
%
%    octave generate_heat_mesh.m > mesh.graph
%


fileName=1;

if fileName~=1
    dst=fopen(fileName,'w');
else
    dst=1;
end

w=127;
h=65;

pixW=min(256,w*3);
pixH=min(128,w*3);
maxTime=ceil(10*sqrt(w*h));
spaceFrac = max(1, w*h/8192);
timeStep=min(1, floor( sqrt(w) ));
minHeat=-8000;
maxHeat=+8000;

devices={};

numEdges=0;

fprintf(2, 'Preparing devices.\n');
id=1;
locToId=zeros(w,h);
for x=1:w
    for y=1:h
        if mod(x+y,2)==0
            continue
        end
        
        nhood=[ x-1 y-1 ; x-2 y ; x-1 y+1 ; x+1 y+1 ; x+2 y ; x+1 y-1];
        % Clip the edges and corners
        nhood=nhood( nhood(:,1)>=1, : );
        nhood=nhood( nhood(:,1)<=w, : );
        nhood=nhood( nhood(:,2)>=1, : );
        nhood=nhood( nhood(:,2)<=h, : );
        
        nhoodSize=size(nhood,1);
        numEdges=numEdges+nhoodSize;
        
        % Work out (completely invented) PDE solution
        selfWeight=rand()*0.2+0.6;
        restWeights=rand(nhoodSize,1)*0.4+0.6;
        restWeights = (1-selfWeight) .* restWeights ./ sum(restWeights);
    
        device.id=id;
        device.rawX=x;
        device.rawY=y;
        device.nhoodSize=nhoodSize;
        device.x=max(0, round(x/w * pixW));
        device.y=max(0, round(y/w * pixW));
        device.selfWeight=selfWeight;
        device.initValue=sin(x/w+y/h);
        device.isDirichlet = (x==1 | x==w | y==1 | y==w);
        device.isOutput = rand() < 1.0/spaceFrac;

        device.inputWeights=restWeights;
        device.inputDelays=floor(rand(nhoodSize,1)*6);
        device.inputSrcsXY=nhood;

        devices{id}=device;
        locToId(x,y)=id;
        id=id+1;
    end
end

numDevices=id-1;
    
fprintf(dst,'POETSGraph\n');
fprintf(dst,'heat\n');
fprintf(dst,'BeginHeader\n');
fprintf(dst,'heat\n');
fprintf(dst,'%u %u\n',numDevices,numEdges);
fprintf(dst,'hex %u %u %u %u %d %d\n', pixW, pixH, maxTime, timeStep, minHeat, maxHeat);
fprintf(dst,'EndHeader\n');


fprintf(dst,'BeginNodes\n');
for i=1:numDevices
    device=devices{i};
    fprintf(dst,'%u %u %u %u %d %d %u %u\n', ...
        device.id, device.nhoodSize,...
        device.x, device.y,...
        round(device.selfWeight*65536), round(device.initValue*65536),...
        device.isDirichlet,device.isOutput...
    );
        
end
fprintf(dst,'EndNodes\n');

    
fprintf(dst,'BeginEdges\n');
for i=1:numDevices
    device=devices{i};
    for j=1:length(device.inputWeights)
        srcXY=device.inputSrcsXY(j,:);
        srcId=locToId(srcXY(1),srcXY(2));
        
        fprintf(dst, '%u %u %u %d\n', device.id-1, srcId-1, device.inputDelays(j), round(device.inputWeights(j)*65536));
    end
end
fprintf(dst,'EndEdges\n');

if dst~=1
    fclose(dst);
end