#define GL_GLEXT_PROTOTYPES

#include "scene.h"
#include "primitive.h"
#include "cube.h"
#include "cylinder.h"
#include "cone.h"
#include "sphere.h"
#include "torus.h"
#include "light.h"

#include "glwidget.h"
#include <QtGui>
#include <GL/gl.h>
#include <iostream>

Scene::Scene(QObject *parent)
	: QAbstractItemModel(parent)
{
	this->modelViewMatrixStack.push(QMatrix4x4());
}
Scene::Scene(GLuint mvLoc, GLuint normalLoc, GLuint idLoc, QObject *parent)
	: QAbstractItemModel(parent)
{
	this->modelViewMatLocation = mvLoc;
	this->normalMatLocation = normalLoc;
	this->idLocation = idLoc;
	this->modelViewMatrixStack.push(QMatrix4x4());

	this->rootDummy = new SceneGraph();
	this->rootNode = new SceneGraph();
	this->rootDummy->add(rootNode);

	lightPosition = QVector4D(0.5, 0.0, 2.0, 1.0);

	//addLight();
	//lights.at(0)->translate(1.0, 3.0, 1.50);
}


QModelIndex Scene::index(int row, int column, const QModelIndex &parent) const {
	if(!hasIndex(row, column, parent)) {
		return QModelIndex();
	}

	SceneGraph *parentNode;

	if(!parent.isValid()) {
		parentNode = rootDummy;
	} else {
		parentNode = static_cast<SceneGraph*>(parent.internalPointer());
	}

	SceneGraph *childNode = parentNode->child(row);
	if(childNode) {
		return createIndex(row, column, childNode);
	} else {
		return QModelIndex();
	}
}

QModelIndex Scene::parent(const QModelIndex &index) const {
	if(!index.isValid()) {
		return QModelIndex();
	}

	SceneGraph *childNode = static_cast<SceneGraph*>(index.internalPointer());
	SceneGraph *parentNode = childNode->parent();

	if(parentNode == rootDummy) {
		return QModelIndex();
	} else {
		return createIndex(parentNode->row(), 0, parentNode);
	}
}

int Scene::rowCount(const QModelIndex &parent) const {
	SceneGraph *parentNode;
	if(parent.column() > 0) {
		return 0;
	}

	if(!parent.isValid()) {
		parentNode = rootDummy;
	} else {
		parentNode = static_cast<SceneGraph*>(parent.internalPointer());
	}
	return parentNode->childCount();
}

int  Scene::columnCount(const QModelIndex &parent) const {
	if(parent.isValid()) {
		return static_cast<SceneGraph*>(parent.internalPointer())->columnCount();
	} else {
		return rootDummy->columnCount();
	}
}

QVariant Scene::data(const QModelIndex &index, int role) const {
	if(!index.isValid()) {
		return QVariant();
	}
	if(role != Qt::DisplayRole) {
		return QVariant();
	}

	SceneGraph *node = static_cast<SceneGraph*>(index.internalPointer());

	return node->data(index.column());
}

Qt::ItemFlags Scene::flags(const QModelIndex &index) const {
	if(!index.isValid()) {
		return 0;
	} else {
		if(static_cast<SceneGraph*>(index.internalPointer())->isLeaf()) {
			return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable |
				Qt::ItemIsDragEnabled;

		} else {
			return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable |
				Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
		}
	}
}

Qt::DropActions Scene::supportedDropActions() const
{
	return Qt::MoveAction;
}

QVariant Scene::headerData(int, Qt::Orientation, int) const {
	return QVariant("Scene Model");
}


bool Scene::setData(const QModelIndex &index, const QVariant &value, int role) {
	if (role != Qt::EditRole) {
		return false;
	}

	SceneGraph *node = static_cast<SceneGraph*>(index.internalPointer());
	bool result = node->setData(index.column(), value);

	if (result) {
		emit dataChanged(index, index);
	}

	return result;
}

bool Scene::insertRows(int position, int rows, const QModelIndex &parent) {
	SceneGraph *parentItem = static_cast<SceneGraph*>(parent.internalPointer());
	bool success;

	beginInsertRows(parent, position, position + rows - 1);
	success = parentItem->insertChildren(position, rows, rootNode->columnCount());
	endInsertRows();

	return success;
}

bool Scene::removeRows(int position, int rows, const QModelIndex &parent) {
	SceneGraph *parentItem = static_cast<SceneGraph*>(parent.internalPointer());
	bool success = true;

	for(int i = 0; i < rows; i++) {
		SceneGraph *toRemove = parentItem->child(position + i);
		unsigned int found = std::find(lights.begin(), lights.end(), toRemove) - lights.begin();
		if(found < lights.size()) {
			lights.erase(lights.begin() + found);
		}
	}
	beginRemoveRows(parent, position, position + rows - 1);
	success = parentItem->removeChildren(position, rows);
	endRemoveRows();

	return success;
}

void Scene::setLightLocation(GLuint lightPositionLocation) {
	this->lightPositionLocation = lightPositionLocation;
}

QModelIndex Scene::identify(int id) {
	auto it = identifier.find(id);
	SceneGraph *s = it->second;
	return createIndex(s->row(), 0, s);
}

QModelIndex Scene::addCube(SceneGraph *node, int) {
	beginResetModel();
	Primitive *cube = new Cube();
	std::string name("Cube ");
	int id = nextId();
	name += std::to_string(id);
	SceneGraph *s = new SceneGraph(cube, name);
	s->setId(id);
	identifier[id] = s;

	node->add(s);
	endResetModel();
	return createIndex(s->row(), 0, s);
}

QModelIndex Scene::addCylinder(SceneGraph *node, int tesselationLevel) {
	beginResetModel();
	Primitive *cylinder = new Cylinder(tesselationLevel);
	std::string name("Cylinder ");
	int id = nextId();
	name += std::to_string(id);
	SceneGraph *s = new SceneGraph(cylinder, name);
	s->setId(id);
	identifier[id] = s;

	node->add(s);
	endResetModel();
	return createIndex(s->row(), 0, s);
}

QModelIndex Scene::addCone(SceneGraph *node, int tesselationLevel) {
	beginResetModel();
	Primitive *cone = new Cone(tesselationLevel);
	std::string name("Cone ");
	int id = nextId();
	name += std::to_string(id);
	SceneGraph *s = new SceneGraph(cone, name);
	s->setId(id);
	identifier[id] = s;

	node->add(s);
	endResetModel();
	return createIndex(s->row(), 0, s);
}

QModelIndex Scene::addSphere(SceneGraph *node, int tesselationLevel) {
	beginResetModel();
	Primitive *sphere = new Sphere(tesselationLevel);
	std::string name("Sphere ");
	int id = nextId();
	name += std::to_string(id);
	SceneGraph *s = new SceneGraph(sphere, name);
	s->setId(id);
	identifier[id] = s;

	node->add(s);
	endResetModel();
	return createIndex(s->row(), 0, s);
}

QModelIndex Scene::addTorus(SceneGraph *node, int tesselationLevel) {
	beginResetModel();
	Primitive *torus = new Torus(tesselationLevel);
	std::string name("Torus ");
	int id = nextId();
	name += std::to_string(id);
	SceneGraph *s = new SceneGraph(torus, name);
	s->setId(id);
	identifier[id] = s;

	node->add(s);
	endResetModel();
	return createIndex(s->row(), 0, s);
}

QModelIndex Scene::addGroup(SceneGraph *node) {
	beginResetModel();
	std::string name("Group ");
	int id = nextId();
	name += std::to_string(id);
	SceneGraph *s = new SceneGraph(false);
	s->setId(id);
	s->setName(name);
	identifier[id] = s;

	node->add(s);
	endResetModel();
	return createIndex(s->row(), 0, s);
}

QModelIndex Scene::addLight() {
	beginResetModel();
	Primitive *light = new Light();
	std::string name("Light ");
	int id = nextId();
	name += std::to_string(id);
	LightNode *s = new LightNode(light, name);
	s->setId(id);
	identifier[id] = s;

	rootNode->add(s);
	lights.push_back(s);
	endResetModel();

	return createIndex(s->row(), 0, s);
}

void Scene::draw(QMatrix4x4 cameraMatrix) {
	modelViewMatrixStack.push(modelViewMatrixStack.top());
	modelViewMatrixStack.top() *= cameraMatrix;

	//Copy the lights positions into GL friendly arrays
	GLfloat *lightsArray = new GLfloat[3 * lights.size()];
	GLfloat *colorsArray = new GLfloat[4 * lights.size()];
	for(unsigned int i = 0; i < lights.size(); i++) {
		QVector3D lightDir = cameraMatrix * lights.at(i)->getPosition();
		lightsArray[3 * i] = lightDir.x();
		lightsArray[3 * i + 1] = lightDir.y();
		lightsArray[3 * i + 2] = lightDir.z();

		GLfloat *color = lights.at(i)->getColor();
		colorsArray[4 * i] = color[0];
		colorsArray[4 * i + 1] = color[1];
		colorsArray[4 * i + 2] = color[2];
		colorsArray[4 * i + 3] = color[3];
	}
	glUniform3fv(shaderProgram->uniformLocation("lightPositions"), lights.size(), lightsArray);
	glUniform4fv(shaderProgram->uniformLocation("lightColors"), lights.size(), colorsArray);
	glUniform1i(shaderProgram->uniformLocation("numLights"), lights.size());

	GLuint colorLocation = shaderProgram->uniformLocation("color");
	this->rootNode->draw(modelViewMatrixStack, modelViewMatLocation, normalMatLocation, idLocation, colorLocation);
	modelViewMatrixStack.pop();

	delete[] lightsArray;
	delete[] colorsArray;
}

void Scene::passLights(QMatrix4x4 cameraMatrix, QOpenGLShaderProgram *sp) {
	//Copy the lights positions into GL friendly arrays
	GLfloat *lightsArray = new GLfloat[3 * lights.size()];
	GLfloat *colorsArray = new GLfloat[4 * lights.size()];
	for(unsigned int i = 0; i < lights.size(); i++) {
		QVector3D lightDir = cameraMatrix * lights.at(i)->getPosition();
		lightsArray[3 * i] = lightDir.x();
		lightsArray[3 * i + 1] = lightDir.y();
		lightsArray[3 * i + 2] = lightDir.z();

		GLfloat *color = lights.at(i)->getColor();
		colorsArray[4 * i] = color[0];
		colorsArray[4 * i + 1] = color[1];
		colorsArray[4 * i + 2] = color[2];
		colorsArray[4 * i + 3] = color[3];
	}
	glUniform3fv(sp->uniformLocation("lightPositions"), lights.size(), lightsArray);
	glUniform4fv(sp->uniformLocation("lightColors"), lights.size(), colorsArray);
	glUniform1i(sp->uniformLocation("numLights"), lights.size());

	delete[] lightsArray;
	delete[] colorsArray;
}

void Scene::lightsPass(QOpenGLShaderProgram *shader, QMatrix4x4 cameraMatrix) {
	for(auto l : lights) {
		glBindFramebuffer(GL_FRAMEBUFFER, l->shadowFBO());

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glClear(GL_DEPTH_BUFFER_BIT);
		glViewport(0,0, 1024, 768);

		glUniformMatrix4fv(shader->uniformLocation("perspectiveMatrix"), 1, GL_FALSE, l->perspectiveMatrix().constData());

		modelViewMatrixStack.push(modelViewMatrixStack.top());
		modelViewMatrixStack.top() *= l->lightView();

		GLuint colorLocation = shader->uniformLocation("color");

		this->rootNode->draw(modelViewMatrixStack, modelViewMatLocation, normalMatLocation, idLocation, colorLocation);

		modelViewMatrixStack.pop();

		//Release and relax, brah
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

template <class COCiter, class Oiter>
void flatten(COCiter start, COCiter end, Oiter dest) {
    using namespace std;
    while (start != end) {
        dest = copy(start->begin(), start->end(), dest);
        ++start;
    }
}

template <typename U, typename T, class UnaryFunction>
std::vector<U> fmap(std::vector<T> v, UnaryFunction f) {
	std::vector<U> w;
	std::transform(v.begin(), v.end(), std::back_inserter(w), f);
	return w;
}

std::vector<GLfloat> Scene::lightPerspectives() {
    using namespace std;
	vector<vector<GLfloat> > views = fmap<vector<GLfloat> >(lights, [=](LightNode *l){
			GLfloat* va = l->perspectiveMatrix().data();
			vector<GLfloat> v;
			v.assign(va, va + 16);
			return v;
		});
	vector<GLfloat> vs;
	flatten(views.begin(), views.end(), back_inserter(vs));
	return vs;
}

std::vector<GLfloat> Scene::lightViews() {
    using namespace std;
	vector<vector<GLfloat> > views = fmap<vector<GLfloat> >(lights, [=](LightNode *l){
			GLfloat* va = l->lightView().data();
			vector<GLfloat> v;
			v.assign(va, va + 16);
			return v;
		});
	vector<GLfloat> vs;
	flatten(views.begin(), views.end(), back_inserter(vs));
	return vs;
}

std::vector<GLuint> Scene::shadowMapLocations() {
	return fmap<GLuint>(lights, [=](LightNode *l){return l->getShadowMap();});
}