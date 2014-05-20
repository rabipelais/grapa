#ifndef SCENE_H
#define SCENE_H

#include "scenegraph.h"
#include "lightnode.h"

#include <stack>
#include <map>
#include <QMatrix4x4>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QOpenGLShader>

class Scene : public QAbstractItemModel
{
    Q_OBJECT
    
private:
    SceneGraph *rootNode;
    SceneGraph *rootDummy;

    
    GLuint modelViewMatLocation;
    GLuint normalMatLocation;
    GLuint lightPositionLocation;
    GLuint idLocation;

    std::map<int, SceneGraph*> identifier;

    std::vector<LightNode*> lights;

    QVector4D lightPosition;
    std::stack<QMatrix4x4> modelViewMatrixStack;

    QOpenGLShaderProgram *shaderProgram;

    int currentId = 0;
    int nextId() {return currentId++;};
    
public:
    Scene(QObject *parent = 0);
    Scene(GLuint mvLoc, GLuint normalLoc, GLuint idLoc, QObject *parent = 0);
    
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    //For editing
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex());
    
    Qt::DropActions supportedDropActions() const;
	
    QModelIndex addCube(SceneGraph *node, int tesselationLevel);
    QModelIndex addCylinder(SceneGraph *node, int tesselationLevel); 
    QModelIndex addCone(SceneGraph *node, int tesselationLevel); 
    QModelIndex addSphere(SceneGraph *node, int tesselationLevel); 
    QModelIndex addTorus(SceneGraph *node, int tesselationLevel);
    
    QModelIndex addGroup(SceneGraph *node);
    
    QModelIndex addLight();

    void setShaderProgram(QOpenGLShaderProgram *sp) {this->shaderProgram = sp;}
    
    SceneGraph *root() {return rootNode;};
    QModelIndex identify(int i);
    void setLightLocation(GLuint lightPositionLocation);
    void draw(QMatrix4x4 cameraMatrix);
};

#endif //SCENE_H
