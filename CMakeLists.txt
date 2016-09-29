PROJECT(Extraction_prothese)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
   
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})
if (ITKVtkGlue_LOADED)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
else()
  find_package(ItkVtkGlue REQUIRED)
  include(${ItkVtkGlue_USE_FILE})
  set(Glue ItkVtkGlue)
endif()

FIND_PACKAGE( Boost COMPONENTS program_options system filesystem REQUIRED )

add_executable(Extraction_prothese Extraction_prothese.cpp) 
target_link_libraries(Extraction_prothese ${Glue} ${VTK_LIBRARIES} ${ITK_LIBRARIES} ${Boost_LIBRARIES} )
