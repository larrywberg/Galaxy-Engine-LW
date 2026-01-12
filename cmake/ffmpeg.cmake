set(HOMEBREW_PREFIX $ENV{HOMEBREW_PREFIX})
if(NOT HOMEBREW_PREFIX AND APPLE)
	set(HOMEBREW_PREFIX /opt/homebrew)
endif()

set(FFMPEG_INCLUDE_HINTS /usr/local/include)
set(FFMPEG_LIB_HINTS /usr/local/lib)
if(HOMEBREW_PREFIX)
	list(APPEND FFMPEG_INCLUDE_HINTS ${HOMEBREW_PREFIX}/include)
	list(APPEND FFMPEG_LIB_HINTS ${HOMEBREW_PREFIX}/lib)
	list(APPEND FFMPEG_INCLUDE_HINTS ${HOMEBREW_PREFIX}/opt/ffmpeg/include)
	list(APPEND FFMPEG_LIB_HINTS ${HOMEBREW_PREFIX}/opt/ffmpeg/lib)
endif()

execute_process(
	COMMAND brew --prefix ffmpeg
	OUTPUT_VARIABLE BREW_FFMPEG_PREFIX
	OUTPUT_STRIP_TRAILING_WHITESPACE
	RESULT_VARIABLE BREW_FFMPEG_RESULT)
if(BREW_FFMPEG_RESULT EQUAL 0 AND BREW_FFMPEG_PREFIX)
	list(APPEND FFMPEG_INCLUDE_HINTS ${BREW_FFMPEG_PREFIX}/include)
	list(APPEND FFMPEG_LIB_HINTS ${BREW_FFMPEG_PREFIX}/lib)
endif()


find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h HINTS ${FFMPEG_INCLUDE_HINTS})

set(FFMPEG_COMPONENTS avcodec avformat avutil swscale swresample)
set(FFMPEG_LIBRARIES)
set(FFMPEG_LIBRARY_FAILURE FALSE)
foreach(component IN LISTS FFMPEG_COMPONENTS)
	string(TOUPPER ${component} component_upper)
	find_library(FFMPEG_${component_upper}_LIB ${component} HINTS ${FFMPEG_LIB_HINTS})
	if(NOT FFMPEG_${component_upper}_LIB)
		set(FFMPEG_LIBRARY_FAILURE TRUE)
	else()
		list(APPEND FFMPEG_LIBRARIES ${FFMPEG_${component_upper}_LIB})
	endif()
endforeach()

if(FFMPEG_INCLUDE_DIR AND NOT FFMPEG_LIBRARY_FAILURE)
	message(STATUS "Using system FFmpeg from found libraries and headers")

	add_library(ffmpeg INTERFACE)
	target_include_directories(ffmpeg INTERFACE ${FFMPEG_INCLUDE_DIR})
	target_link_libraries(ffmpeg INTERFACE ${FFMPEG_LIBRARIES})
else()
	message(STATUS "System FFmpeg not found; falling back to bundled download")
	set(FFMPEG_URL_BASE https://github.com/BtbN/FFmpeg-Builds/releases/download/latest)

	if(LINUX)
		set(FFMPEG_ARCHIVE ffmpeg-master-latest-linux64-lgpl-shared.tar.xz)
	elseif(APPLE)
		set(FFMPEG_ARCHIVE ffmpeg-master-latest-macos64-lgpl-shared.tar.xz)
	elseif(WIN32)
		set(FFMPEG_ARCHIVE ffmpeg-master-latest-win64-lgpl-shared.zip)
	endif()

	FetchContent_Declare(ffmpeg-fetch URL ${FFMPEG_URL_BASE}/${FFMPEG_ARCHIVE})
	FetchContent_MakeAvailable(ffmpeg-fetch)

	add_library(ffmpeg INTERFACE)

	target_include_directories(ffmpeg INTERFACE ${ffmpeg-fetch_SOURCE_DIR}/include)

	target_link_directories(ffmpeg INTERFACE ${ffmpeg-fetch_SOURCE_DIR}/lib)
	target_link_directories(ffmpeg INTERFACE ${ffmpeg-fetch_SOURCE_DIR}/bin)
	target_link_libraries(ffmpeg INTERFACE avcodec avformat avutil swscale swresample)
endif()
