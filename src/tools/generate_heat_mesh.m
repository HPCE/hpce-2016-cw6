% Minimal example of generating a mesh. You can run it using
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

numCell=1024;

pixW=max(512,5*sqrt(numCell));
pixH=pixW;
maxTime=numCell;
spaceStep=max(1, floor(numCell/8192));
timeStep=max(1, floor( maxTime/100 ));
minHeat=-10000;
maxHeat=+10000;

numBoundary=floor(sqrt(8*numCell));
u=(0:numBoundary-1)'./numBoundary;
bx=sin( u*2*pi);
by=cos( u*2*pi);

ix=rand(numCell,1)*2-1;
iy=rand(numCell,1)*2-1;

in=(ix.^2+iy.^2) < 1;
ix=ix(in);
iy=iy(in);

ia=atan2(ix,iy);
im=sqrt(ix.^2+iy.^2);
im=sqrt(im);
ix=im.*sin(ia);
iy=im.*cos(ia);

numCell=length(ix);

x=[bx ; ix];
y=[by ; iy];

fprintf(2, 'Triangulating.\n');
triangles=delaunay(x,y);

if 0
    triplot(triangles, x,y)
end

numDevices=numBoundary+numCell;

fprintf(2, 'Extracting channels.\n');
% Get all the unique edges
edges=[...
    [triangles(:,1) , triangles(:,2)] ; ...
    [triangles(:,1) , triangles(:,3)] ; ...
    [triangles(:,2) , triangles(:,3)]
];

% Sort them into low < high
swap=edges(:,1) > edges(:,2);
edges(swap,:) = [edges(swap,2) , edges(swap,1) ];
edges=unique(edges,'rows');

numEdges=2*size(edges,1);

fprintf(dst,'POETSGraph\n');
fprintf(dst,'heat\n');
fprintf(dst,'BeginHeader\n');
fprintf(dst,'heat\n');
fprintf(dst,'%u %u\n',numDevices,numEdges);
fprintf(dst,'mesh %u %u %u %u %d %d\n', pixW, pixH, maxTime, timeStep, minHeat, maxHeat);
fprintf(dst,'EndHeader\n');

devices={};

fprintf(2, 'Preparing devices.\n');
for i=1:numDevices
    if mod(i,1000)==0
        fprintf(2, '  device %u out of %u.\n', i, numDevices);
    end

    % Find all the outgoing arcs
    hits = (edges(:,1)==i) | (edges(:,2)==i);
    local = edges(hits,:);
    swap = local(:,1) ~= i;
    local(swap,:) = [ local(swap,2) , local(swap,1) ];

    % Calculate distances
    nhoodSize = size(local,1);
    dists = sqrt( (x(local(:,1))-x(local(:,2))).^2 + (y(local(:,1))-y(local(:,2))).^2 );
    normDists = dists ./ sum(dists); % Normalise

    % Work out (completely invented) PDE solution
    selfWeight=rand()*0.2+0.6;
    restWeights = (1-selfWeight) .* (1-normDists) ./ sum(1-normDists);

    device.id=i;
    device.nhoodSize=nhoodSize;
    device.x=max(0, round(pixW * (x(i)/2+0.5) ));
    device.y=max(0, round(pixH * (y(i)/2+0.5) ));
    device.selfWeight=selfWeight;
    device.initValue=sin(x(i)+y(i));
    device.isDirichlet = i<=numBoundary;
    device.isOutput = rand() < 1.0/spaceStep;

    device.inputWeights=restWeights;
    device.inputDelays=floor(4 * (dists./max(dists)));
    device.inputSrcs=local(:,2);

    devices{i}=device;
end

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
        fprintf(dst, '%u %u %u %d\n', i-1, device.inputSrcs(j)-1, device.inputDelays(j), round(device.inputWeights(j)*65536));
    end
end
fprintf(dst,'EndEdges\n');

if dst~=1
    fclose(dst);
end
