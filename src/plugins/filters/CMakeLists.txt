add_library(filters_internal STATIC
    filtercolumnregistry.cpp
    filtercolumnregistry.h
    filtercolumneditordialog.cpp
    filtercolumneditordialog.h
    filterscolumnmodel.cpp
    filterscolumnmodel.h
    filterconstants.h
    filtercontroller.cpp
    filtercontroller.h
    filtercontextmenu.h
    filterdelegate.cpp
    filterdelegate.h
    filterfwd.h
    filteritem.cpp
    filteritem.h
    filtermanager.cpp
    filtermanager.h
    filtermodel.cpp
    filtermodel.h
    filterpipeline.cpp
    filterpipeline.h
    filterrows.cpp
    filterrows.h
    filterwidget.cpp
    filterwidget.h
    filterconfigwidget.cpp
    filterconfigwidget.h
)
add_library(Fooyin::FiltersInternal ALIAS filters_internal)

target_link_libraries(filters_internal PRIVATE Fooyin::Gui)
target_include_directories(filters_internal
    PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}
)
set_target_properties(filters_internal
    PROPERTIES POSITION_INDEPENDENT_CODE ON
               CXX_VISIBILITY_PRESET hidden
               VISIBILITY_INLINES_HIDDEN YES
)
target_compile_features(filters_internal PUBLIC ${FOOYIN_REQUIRED_CXX_FEATURES})
target_compile_definitions(filters_internal PRIVATE ${FOOYIN_COMPILE_DEFINITIONS})
target_compile_options(filters_internal PRIVATE ${FOOYIN_COMPILE_OPTIONS})
target_link_options(filters_internal INTERFACE ${FOOYIN_LINK_OPTIONS})
if (BUILD_PCH)
    target_precompile_headers(filters_internal REUSE_FROM fooyin_pch)
endif ()

create_fooyin_plugin_internal(
    filters
    DEPENDS Fooyin::Gui
            filters_internal
    SOURCES filtersplugin.cpp
            filtersplugin.h
)
