#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>
#include <QMatrix4x4>
#include <QOpenGLShader>

class GLWidget : public QGLWidget
{
    Q_OBJECT

public:
    GLWidget(QWidget *parent = 0);
    ~GLWidget();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

private:
    int tesselationLevel;
    void drawCube();

    QQuaternion cubeRotationQuat;
    float zoom;
    float xtrans;
    float ytrans;
    bool dragging;

    QPoint lastPoint;

    QOpenGLShaderProgram *shaderProgram;
    QOpenGLShader *vshader;
    QOpenGLShader *fshader;
    void loadShaders(QString vshader, QString fshader);
    
protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent (QWheelEvent *event);

public slots:
    void setWireframeShading();
    void setFlatShading();
    void setGouraudShading();
    void setPhongShading();
    void setTesselation(int tesselationLevel);
    void resetCamera();
};

#endif
