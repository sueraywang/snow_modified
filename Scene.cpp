#include "Scene.h"

Scene::Scene() {}
Scene::Scene(const Scene& scene) {}
Scene::~Scene() {}

Scene* Scene::snowballSmash() {

	Scene* scene = new Scene();

	Shape* snowball = Shape::generateSnowball(Vector2f(0.8, .5), .1, Vector2f(0, 0));
	scene->snow_shapes.push_back(snowball);

	Shape* snowball2 = Shape::generateSnowball(Vector2f(.2, .5), .1, Vector2f(10, 0));
	scene->snow_shapes.push_back(snowball2);

	return scene;
}