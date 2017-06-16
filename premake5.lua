
local core_project = 'core/'
local client_project = 'host/'
local render_project = 'render_client/'

-- go through opencl vendors env variables for opencl.lib locations

local amd_root = os.getenv('AMDAPPSDKROOT', '')
local nv_root = os.getenv('CUDA_PATH', '')
local intel_root = os.getenv('INTELOCLSDKROOT', '')

local opencl_libdir = ''
 

-- find opencl

if (amd_root ~= '' and amd_root ~= nil) then
  opencl_libdir = amd_root .. '\\lib\\x86_64'
  print('Found amd sdk at: ' .. amd_root)
elseif (intel_root ~= '' and intel_root ~= nil) then
  opencl_libdir = intel_root .. '\\lib\\x64'
  print('Found intel opencl sdk at: ' .. intel_root)
elseif (nv_root ~= '' and nv_root ~= nil) then
  opencl_libdir = nv_root .. '\\lib\\x64'
  print('Found nvidia cuda sdk at: ' .. nv_root)
else
  print('Could not find an OpenCL sdk.  Renderer client project may not link.');
end


-- link to glfw, glew
function link_to_gls(use_core)
  local libdir = 'libs';
  local raycore_dir_rel = 'lib\\Release'
  local raycore_dir_deb = 'lib\\Debug'
  
  filter { 'system:windows', 'configurations:debug' }
    libdirs { libdir .. '\\Win32', raycore_dir_deb }
  filter { 'system:windows', 'configurations:release' }
		libdirs { libdir .. '\\Win32', raycore_dir_rel }
  filter { 'system:linux' }
    libdirs { libdir .. '\\Unix' }
  filter {}
  
  links { 'glfw3', 'glew32' }
  if (use_core) then
    links { 'raycore' }
  end
  
end

function include_dirs()

  includedirs { 'include' }

end


workspace 'raytracer'
  configurations { 'Debug', 'Release' }
  location 'build'
  startproject 'render_client'
  architecture 'x64'

  
project 'core'
  kind 'StaticLib'
  language 'C++'
  targetdir 'lib/%{cfg.buildcfg}'
  targetname 'raycore'
  
  files { core_project .. '**.c', core_project .. '**.h', core_project .. '**.cpp', core_project .. '**.hpp' }

  include_dirs()
  
  libdirs { opencl_libdir }
  links { 'opencl' }
  
  link_to_gls(false)

project 'host'
  kind 'ConsoleApp'
  language 'C++'
  targetdir 'bin/%{cfg.buildcfg}'
  
  files { client_project .. '**.c', client_project .. '**.h', client_project .. '**.cpp', client_project .. '**.hpp' }
  
  include_dirs()
  includedirs(core_project)
  
  link_to_gls(true)
  links { 'OpenGL32' }
  
project 'render_client'
  kind 'ConsoleApp'
  language 'C++'
  targetdir 'bin/%{cfg.buildcfg}'

  files { render_project .. '**.c', render_project .. '**.h', render_project .. '**.cpp', render_project .. '**.hpp' }
  
  include_dirs()
  includedirs(core_project)
  
  libdirs { opencl_libdir }
  links { 'opencl' }
  
  link_to_gls(true)
  links { 'OpenGL32' }
  
  