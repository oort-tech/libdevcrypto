file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

add_library(devcrypto ${SOURCES} ${HEADERS})
target_link_libraries(devcrypto PUBLIC devcore PRIVATE libff::ff)
