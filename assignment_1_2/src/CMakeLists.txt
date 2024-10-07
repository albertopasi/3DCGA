target_sources(ParticleSimLib
	PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/render/mesh.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/simulation/particles.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/simulation/sphere_container.cpp"
        
        "${CMAKE_CURRENT_LIST_DIR}/ui/camera.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ui/menu.cpp")
