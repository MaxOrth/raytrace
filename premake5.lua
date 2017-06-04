
local core_project = 'core/'
local client_project = 'host/'
local render_project = 'render_client/'

-- go through opencl vendors env variables for opencl.lib locations

local amd_root = os.getenv('AMDAPPSDKROOT', '')
local nv_root = os.getenv('CUDA_PATH', '')
local intel_root = os.getenv('INTELOCLSDKROOT', '')

local opencl_libdir = ''

-- find opencl

if (amd_root ~= '') then
  opencl_libdir = amd_root .. '\lib\x86_64'
elseif (intel_root ~= '') then
  opencl_libdir = intel_root .. '\lib\x64'
elseif (nv_root ~= '') then
  opencl_libdir = nv_root .. '\lib\x64'
else
  print('Could not find an OpenCL sdk.  Renderer client project may not link.');
end


-- link to glfw, glew
function link_to_gls()
  local libdir = 'libs';
  
  filter { 'platforms:windows' }
    libdirs { libdir .. '/Win32/' }
    
  filter { 'platforms:linux' }
    libdirs { libdir .. '/Unix/' }
  
  filter {}
  
  links { 'glfw3', 'glew_s' }
  
end

function include_dirs()

  includedirs { 'include' }

end


workspace 'raytracer'
  configurations { 'Debug', 'Release' }
  location 'build'

  architecture 'x64'

project 'core'
  kind 'StaticLib'
  language 'C++'
  targetdir 'lib/%{cfg.buildcfg}'
  
  files { core_project .. '**.c', core_project .. '**.h', core_project .. '**.cpp', core_project .. '**.hpp' }

  include_dirs()
  
  libdirs { opencl_libdir }
  links { 'opencl' }
  
  link_to_gls()

project 'host'
  kind 'ConsoleApp'
  language 'C++'
  targetdir 'bin/%{cfg.buildcfg}'
  
  files { client_project .. '**.c', client_project .. '**.h', client_project .. '**.cpp', client_project .. '**.hpp' }
  
  include_dirs()
  includedirs(core_project)
  
  link_to_gls()
  
project 'render_client'
  kind 'ConsoleApp'
  language 'C++'
  targetdir 'bin/%{cfg.buildcfg}'

  files { render_project .. '**.c', render_project .. '**.h', render_project .. '**.cpp', render_project .. '**.hpp' }
  
  include_dirs()
  includedirs(core_project)
  
  libdirs { opencl_libdir }
  links { 'opencl' }
  
  link_to_gls()
  
  