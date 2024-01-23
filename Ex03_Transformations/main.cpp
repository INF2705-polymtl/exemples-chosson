#include <cstddef>
#include <cstdint>

#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#define GLM_FORCE_SWIZZLE // Pour utiliser les .xyz .rb etc. comme avec GLSL.
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <inf2705/OpenGLApplication.hpp>
#include <inf2705/Mesh.hpp>
#include <inf2705/TransformStack.hpp>
#include <inf2705/ShaderProgram.hpp>


using namespace gl;
using namespace glm;


// Des couleurs
vec4 black = {0.0f, 0.0f, 0.0f, 1.0f};
vec4 darkGrey = {0.2f, 0.2f, 0.2f, 1.0f};
vec4 grey = {0.5f, 0.5f, 0.5f, 1.0f};
vec4 white = {1.0f, 1.0f, 1.0f, 1.0f};
vec4 brightRed = {1.0f, 0.2f, 0.2f, 1.0f};
vec4 brightGreen = {0.2f, 1.0f, 0.2f, 1.0f};
vec4 brightBlue = {0.2f, 0.2f, 1.0f, 1.0f};
vec4 brightYellow = {1.0f, 1.0f, 0.2f, 1.0f};


struct App : public OpenGLApplication
{
	void init() override {
		// Config de base, pas de cull, lignes assez visibles.
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_POINT_SMOOTH);
		glPointSize(3.0f);
		glLineWidth(3.0f);
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);

		loadShaders();
		initPyramid();
		initCube();

		//setupSquareCamera();
		applyPerspective();
	}

	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		setupOrbitCamera();
		drawCubeScene();

		drawRotatingPyramid();

		rotatingAngle += 2;
	}

	void onKeyEvent(const sf::Event::KeyEvent& key) override {
		using enum sf::Keyboard::Key;
		
		switch (key.code) {
		// Les touches + et - servent à rapprocher et éloigner la caméra orbitale.
		case Add:
			cameraDistance -= 0.5f;
			if (not perspectiveCamera)
				// Avec une projection orthogonale, changer la distance de la caméra ne change pas le champs de vision, car celui-ci est contrôlé par la boîte de projection. Il faut donc changer la boîte de projection quand on change la distance de la caméra orbitale.
				applyOrtho();
			break;
		case Subtract:
			cameraDistance += 0.5f;
			if (not perspectiveCamera)
				applyOrtho();
			break;
		// Les touches haut/bas change l'élévation ou la latitude de la caméra orbitale.
		case Up:
			cameraLatitude += 5.0f;
			break;
		case Down:
			cameraLatitude -= 5.0f;
			break;
		// Les touches gauche/droite change l'azimuth ou la longitude de la caméra orbitale.
		case Left:
			cameraLongitude += 5.0f;
			break;
		case Right:
			cameraLongitude -= 5.0f;
			break;
		// Réinitialiser la position de la caméra.
		case R:
			cameraDistance = 5;
			cameraLatitude = 0;
			cameraLongitude = 0;
			break;
		// 1 : Projection perspective
		case Num1:
			perspectiveCamera = true;
			updateProjection();
			break;
		// 2 : Projection orthogonale
		case Num2:
			perspectiveCamera = false;
			updateProjection();
			break;
		}
	}

	void onResize(const sf::Event::SizeEvent& event, const sf::Vector2u& oldSize) override {
		// Mettre à jour la matrice de projection avec le nouvel aspect de fenêtre après le redimensionnement.
		updateProjection();
	}

	void loadShaders() {
		// Créer le programme nuanceur pour les couleurs par sommet.
		coloredVertexShaders.create();
		// Compiler et attacher le nuanceur de sommets. Il sera réutilisé
		auto basicVertexShader = coloredVertexShaders.attachSourceFile(GL_VERTEX_SHADER, "basic_vert.glsl");
		// Compiler et attacher le nuanceur de fragments qui utilise une couleur d'entrée.
		coloredVertexShaders.attachSourceFile(GL_FRAGMENT_SHADER, "vertex_color_frag.glsl");
		coloredVertexShaders.link();

		// Créer le programme nuanceur pour les couleurs solides.
		solidColorShaders.create();
		// Attacher le nuanceur de sommets précédent pour ne pas avoir à le recompiler.
		solidColorShaders.attachExistingShader(GL_VERTEX_SHADER, basicVertexShader);
		// Compiler et attacher le nuanceur de fragments qui utilise une couleur uniforme.
		solidColorShaders.attachSourceFile(GL_FRAGMENT_SHADER, "solid_color_frag.glsl");
		solidColorShaders.link();
	}

	void initPyramid() {
		vec3 top =     { 0.0f,  0.3f,  0.0f};
		vec3 bottomR = {-0.3f, -0.1f, -0.1f};
		vec3 bottomL = { 0.3f, -0.1f, -0.1f};
		vec3 bottomF = { 0.0f, -0.1f,  0.7f};
		pyramid.vertices = {
			{top,     {}, {}},
			{bottomR, {}, {}},
			{bottomL, {}, {}},
			{bottomF, {}, {}},
		};
		pyramid.indices = {
			// Dessous
			1, 2, 3,
			// Face "tribord"
			1, 3, 0,
			// Face "babord"
			3, 2, 0,
			// Face arrière
			2, 1, 0,
		};
		pyramid.setup();

		// La classe Mesh configure les attributs de position, de normale et de coordonnées de texture pour chaque sommet. Ce sont pas mal les informations de base nécessaires pour afficher des formes. Dans notre cas, on veut de la couleur, donc un vec4 (RGBA) de plus pour chaque sommet. On peut ajouter un autre VBO qui passera les couleurs de sommets dans l'attribut de sommet à l'index 3.
		vec4 pyramidVertexColors[4] = {
			brightYellow,
			brightGreen,
			brightRed,
			brightBlue,
		};
		// Activer le contexte du VAO de la pyramide.
		pyramid.bindVao();
		// Créer et passer les données au VBO des couleurs.
		glGenBuffers(1, &pyramidColorVbo);
		glBindBuffer(GL_ARRAY_BUFFER, pyramidColorVbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVertexColors), pyramidVertexColors, GL_STATIC_DRAW);
		// Configurer l'attribut 3 avec des vec4.
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(3);
	}

	void drawRotatingPyramid() {
		model.pushIdentity();
		// L'ordre des opérations est important. On veut faire un « barrel roll » (Starfox, do a barrel roll!). Il faut donc faire la rotation en premier puis faire la translation.
		model.rotate(rotatingAngle, {0, 0, 1});
		model.translate({0, -0.5f, 0});
		//model.scale({1, 1.5f, 1});
		// Les programmes de nuanceur ont leur propre espace mémoire. Il faut donc mettre à jour les variables uniformes pour chaque programme.
		coloredVertexShaders.use();
		coloredVertexShaders.setMat("model", model);
		solidColorShaders.use();
		solidColorShaders.setMat("model", model);
		model.pop();

		// Dessiner la pyramide avec le nuanceur avec couleur par sommet.
		coloredVertexShaders.use();
		pyramid.draw(GL_TRIANGLES);
		solidColorShaders.use();
		// Redessiner la pyramide avec le nuanceur avec couleur globale et en dessinant des lignes.
		solidColorShaders.setVec("globalColor", black);
		pyramid.draw(GL_LINE_STRIP);
	}

	void initCube() {
		vec3 backBottomR =  {-1, -1, -1};
		vec3 backTopR =     {-1,  1, -1};
		vec3 backBottomL =  { 1, -1, -1};
		vec3 backTopL =     { 1,  1, -1};
		vec3 frontBottomR = {-1, -1,  1};
		vec3 frontTopR =    {-1,  1,  1};
		vec3 frontBottomL = { 1, -1,  1};
		vec3 frontTopL =    { 1,  1,  1};

		cube.vertices = {
			{backBottomR, {}, {}},
			{backTopR, {}, {}},
			{backBottomL, {}, {}},
			{backTopL, {}, {}},
			{frontBottomR, {}, {}},
			{frontTopR, {}, {}},
			{frontBottomL, {}, {}},
			{frontTopL, {}, {}},
		};
		cube.indices = {
			// Faces arrière
			0, 1, 3,
			0, 2, 3,
			// Faces droite
			4, 5, 1,
			4, 0, 1,
			// Faces gauche
			2, 3, 7,
			2, 6, 7,
			// Faces avant
			6, 7, 5,
			6, 4, 5,
			// Faces dessus
			7, 3, 1,
			7, 5, 1,
			// Faces dessous
			6, 4, 0,
			6, 2, 0,
		};
		cube.setup();

		// Configurer un deuxième mesh pour le wireframe du cube, de cette façon on ne dessine pas les lignes des triangles dans les faces et on dessine seulement les arêtes du cube.
		cubeWire.vertices = cube.vertices;
		cubeWire.indices = {
			// Face arrière
			0, 1, 1, 3, 3, 2, 2, 0,
			// Face avant
			4, 5, 5, 7, 7, 6, 6, 4,
			// Arêtes de côté
			0, 4, 1, 5, 3, 7, 2, 6
		};
		cubeWire.setup();
	}

	void drawCubeScene() {
		solidColorShaders.use();
		model.pushIdentity();
		model.translate({0, -1, 0});

		model.push();
		model.translate({0, 0, 0});
		model.scale({1.1f, 0.1f, 1.1f});
		solidColorShaders.setMat("model", model);
		solidColorShaders.setVec("globalColor", grey);
		cube.draw(GL_TRIANGLES);
		solidColorShaders.setVec("globalColor", black);
		cubeWire.draw(GL_LINES);
		model.pop();

		model.push();
		model.translate({-0.7f, 0.5f, -0.7f});
		model.rotate(rotatingAngle, {0, 1, 0});
		model.scale({0.2f, 0.5f, 0.2f});
		solidColorShaders.setMat("model", model);
		solidColorShaders.setVec("globalColor", brightRed);
		cube.draw(GL_TRIANGLES);
		solidColorShaders.setVec("globalColor", black);
		cubeWire.draw(GL_LINES);
		model.pop();

		model.push();
		model.translate({0.5f, 0.2f, 0.5f});
		model.scale({0.4f, 0.2f, 0.4f});
		solidColorShaders.setMat("model", model);
		solidColorShaders.setVec("globalColor", brightGreen);
		cube.draw(GL_TRIANGLES);
		solidColorShaders.setVec("globalColor", black);
		cubeWire.draw(GL_LINES);
		model.pop();

		model.pop();
	}

	void setupSquareCamera() {
		// Configuration de caméra qui donne une projection perspective où le plan z=0 a une boîte -1 à 1 en x, y.
		// Ça ressemble donc à la projection par défaut (orthogonale dans une boîte -1 à 1) mais avec une perspective.

		// La matrice de visualisation positionne la caméra dans l'espace.
		view.lookAt({0.0f, 0.0f, 6.0f}, {0.0f, 0.0f, 0.0f}, {0, 1, 0});
		// La matrice de projection contrôle comment la caméra observe la scène.
		projection.frustum({-2.0f / 3, 2.0f / 3, -2.0f / 3, 2.0f / 3, 4.0f, 10.0f});

		setCameraMatrix("view", view);
		setCameraMatrix("projection", projection);
	}

	void setupOrbitCamera() {
		view.pushIdentity();
		// Comme pour le reste, l'ordre des opérations est important.
		view.translate({0, 0, -cameraDistance});
		view.rotate(cameraRoll,      {0, 0, 1});
		view.rotate(cameraLatitude,  {1, 0, 0});
		view.rotate(cameraLongitude, {0, 1, 0});
		// En positionnant la caméra, on met seulement à jour la matrice de visualisation.
		setCameraMatrix("view", view);
		view.pop();
	}

	void applyPerspective() {
		// On calcule l'aspect de notre caméra à partir des dimensions de la fenêtre.
		auto windowSize = window_.getSize();
		float aspect = (float)windowSize.x / windowSize.y;

		projection.pushIdentity();
		projection.perspective(50, aspect, 0.1f, 100.0f);
		setCameraMatrix("projection", projection);
		projection.pop();
	}

	void applyOrtho() {
		auto windowSize = window_.getSize();
		float aspect = (float)windowSize.x / windowSize.y;

		projection.pushIdentity();
		float boxEdge = cameraDistance / 3;
		ProjectionBox box = {-boxEdge, boxEdge, -boxEdge, boxEdge, 0.1f, 100.0f};
		box.leftFace *= aspect;
		box.rightFace *= aspect;
		projection.ortho(box);
		setCameraMatrix("projection", projection);
		projection.pop();
	}

	void updateProjection() {
		if (perspectiveCamera)
			applyPerspective();
		else
			applyOrtho();
	}

	void setCameraMatrix(std::string_view name, const TransformStack& matrix) {
		coloredVertexShaders.use();
		coloredVertexShaders.setMat(name, matrix);
		solidColorShaders.use();
		solidColorShaders.setMat(name, matrix);
	}

	Mesh pyramid;
	Mesh cube;
	Mesh cubeWire;
	GLuint pyramidColorVbo = 0;
	ShaderProgram coloredVertexShaders;
	ShaderProgram solidColorShaders;

	TransformStack model;
	TransformStack view;
	TransformStack projection;

	bool perspectiveCamera = true;
	float cameraDistance = 5;
	float cameraRoll = 0;
	float cameraLatitude = 0;
	float cameraLongitude = 0;

	float rotatingAngle = 0;
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.fps = 30;
	settings.context.antialiasingLevel = 4;

	App app;
	app.run(argc, argv, "Exemple Semaine 3: Transformations", settings);
}
