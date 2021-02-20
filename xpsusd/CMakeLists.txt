project(xpsusd)
add_executable(x2u "src/xpsusd.cpp" "src/xps_reader.cpp")
message(${PXR_INCLUDE_DIRS})
target_include_directories(x2u
    PUBLIC include ${PXR_INCLUDE_DIRS}
    PRIVATE inc
)
set(pxr_libs
    "arch;tf;gf;js;trace;work;plug;vt;ar;kind;sdf;ndr;sdr;pcp;usd;usdGeom;usdVol;usdLux;usdMedia;usdShade;usdRender;usdHydra;usdRi;usdSkel;usdUI;usdUtils;garch;hf;")
string(REPLACE " " ";" pxr_libs "${pxr_libs}")
target_link_libraries(x2u
    PUBLIC ${pxr_libs}
    PRIVATE argparse::argparse
)

file(GLOB copy_dlls "${PXR_CMAKE_DIR}/lib/*.dll")
foreach(dll ${copy_dlls})
    add_custom_command(TARGET x2u POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${dll} $<TARGET_FILE_DIR:x2u>/)
endforeach()
add_custom_command(TARGET x2u POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PXR_CMAKE_DIR}/lib/usd $<TARGET_FILE_DIR:x2u>/usd)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DTBB_USE_DEBUG")
