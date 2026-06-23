contains(CONFIG, robotkinematics_mesh_collision) {
    isEmpty(MESH_COLLISION_BACKEND): MESH_COLLISION_BACKEND = $$(MESH_COLLISION_BACKEND)
    isEmpty(COAL_ROOT): COAL_ROOT = $$(COAL_ROOT)
    isEmpty(BOOST_ROOT): BOOST_ROOT = $$(BOOST_ROOT)
    isEmpty(ASSIMP_ROOT): ASSIMP_ROOT = $$(ASSIMP_ROOT)

    isEmpty(MESH_COLLISION_BACKEND) {
        error(MESH_COLLISION_BACKEND must be set when CONFIG+=robotkinematics_mesh_collision)
    }

    equals(MESH_COLLISION_BACKEND, coal) {
        isEmpty(COAL_ROOT) {
            error(COAL_ROOT must point to the Coal install root when MESH_COLLISION_BACKEND=coal)
        }
        isEmpty(BOOST_ROOT) {
            error(BOOST_ROOT must point to the Boost install root when MESH_COLLISION_BACKEND=coal)
        }

        DEFINES += ROBOTKINEMATICS_HAVE_COAL_MESH_BACKEND
        INCLUDEPATH += \
            $$COAL_ROOT/include \
            $$BOOST_ROOT/include/boost-1_87

        !isEmpty(ASSIMP_ROOT) {
            INCLUDEPATH += $$ASSIMP_ROOT/include
        }
        LIBS += -L$$COAL_ROOT/lib -lcoal
    } else {
        error(Unsupported MESH_COLLISION_BACKEND value: $$MESH_COLLISION_BACKEND)
    }
}
