ADD_DEFINITIONS(-DDEDICATED_CLIENT ${PIC_FLAG} -DNO_AVI -DNOT_USING_CREG)
remove_definitions(-DSTREFLOP_SSE)

AUX_SOURCE_DIRECTORY(../../rts/System/Net/ netfiles)
INCLUDE_DIRECTORIES(../../rts/System/Net/ ../../rts/System/)
INCLUDE_DIRECTORIES(../../rts/)
include_directories(${GLEW_INCLUDE_DIR})


SET(system_files
	${netfiles}
	../../rts/System/Platform/Misc
	../../rts/System/TdfParser
	../../rts/System/ConfigHandler
	../../rts/System/LogOutput
	../../rts/System/NetProtocol
	../../rts/System/TimeUtil
	../../rts/System/BaseNetProtocol
	../../rts/System/Demo
	../../rts/System/DemoRecorder
	../../rts/System/GlobalUnsynced
	../../rts/Game/GameSetup
	../../rts/Game/GameData
    ../../rts/Game/ClientSetup
	../../rts/Game/PlayerBase
	../../rts/Game/GameVersion
	../../rts/Sim/Misc/TeamBase
	../../rts/Sim/Misc/AllyTeam
	../../rts/Rendering/Textures/TAPalette)
IF (WIN32)
	SET(system_files
		${system_files}
		../../rts/System/Platform/Win/WinVersion)
ENDIF (WIN32)

ADD_EXECUTABLE(spring-dedicated-client main ${system_files})
TARGET_LINK_LIBRARIES(spring-dedicated-client ${SDL_LIBRARY} ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_SYSTEM_LIBRARY} hpiutil2 )
if (MINGW)
	TARGET_LINK_LIBRARIES (spring-dedicated-client ws2_32)
else (MINGW)
	set_target_properties(spring-dedicated-client PROPERTIES COMPILE_FLAGS "-fvisibility=default")
endif (MINGW)

install (TARGETS spring-dedicated-client RUNTIME DESTINATION ${BINDIR} )
