if not exist "%cd%/shader" mkdir "%cd%/shader"

%VULKAN_SDK%/Bin/glslc sandbox/shader/shader.vert -o shader/vert.spv
%VULKAN_SDK%/Bin/glslc sandbox/shader/shader.frag -o shader/frag.spv