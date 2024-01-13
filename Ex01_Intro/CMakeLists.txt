cmake_minimum_required(VERSION 3.0.0)
# La raison pour laquelle on fait une variable d'environnement VCPKG_ROOT.
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

# Le nom du projet.
project(INF2705_Ex01_Intro)

# On met les fichiers sources (incluant les entêtes)
set(ALL_FILES
    "main.cpp"
)
add_executable(${PROJECT_NAME} ${ALL_FILES})

include_directories("../")

# Les flags de compilation.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fsigned-char -Wno-unknown-pragmas -Wno-enum-compare")

# Tous ce qui suit sont des package Vcpkg. Pour savoir quoi mettre, on fait "vcpkg install <le package>", puis on met ce qu'ils disent de mettre dans le CMakeLists.
# Vous pouvez commenter/décommenter les packages que vous utilisez ou pas.

# GLM: Pour les math comme en GLSL.
find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)

# SFML: Pour la gestion de fenêtre et d'événements.
#       Tout en C++, assez clean et léger.
find_package(SFML COMPONENTS system window graphics CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-system sfml-network sfml-graphics sfml-window)

# glbinding: Pour l'importation des fonctions OpenGL et la résolution d'adresses.
#            Tout en C++ assez moderne, très clean avec des enum, des namespace et peu de macros.
find_package(glbinding CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glbinding::glbinding glbinding::glbinding-aux)