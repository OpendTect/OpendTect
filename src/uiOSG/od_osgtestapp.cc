#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMdiArea>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QOpenGLWidget>
#include <QMdiSubWindow>
#include <QSurfaceFormat>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <osg/Camera>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Group>
#include <osg/LineWidth>
#include <osg/Matrix>
#include <osg/ShapeDrawable>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>

#include <iostream>

class OSGWidget;

class OSGWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit OSGWindow(QWidget* parent=nullptr);
    ~OSGWindow();

    void setOSGWidget(OSGWidget*);

private:
    OSGWidget*	osgwidget_	= nullptr;

private slots:
    void onOpen();
    void onSave();
    void onTakeScreenshot();
    void onExit();

};


class OSGWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    OSGWidget(QWidget* parent=nullptr);
    ~OSGWidget();

    void doScreenShot(int res) const;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;

private:
    osgGA::GUIEventAdapter::MouseButtonMask mapQtMouseButton(Qt::MouseButton);
    int mapQtKey(QKeyEvent*);

    osg::ref_ptr<osgViewer::Viewer> viewer_;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> gw_;
    osg::ref_ptr<osg::Group> root_;
    QTimer* rendertimer_	= nullptr;
    osg::Matrix lastviewmatrix_;
    int stillframecount_	= 0;
    const int maxstillframes_ = 10;
};

#include "od_osgtestapp.moc"


// OSGWindow

OSGWindow::OSGWindow(QWidget* parent)
    : QMainWindow(parent)
{
    auto* menuBar = new QMenuBar;
    QMenu* fileMenu = menuBar->addMenu("&File");

    QAction* openact = fileMenu->addAction("Open...");
    QAction* saveact = fileMenu->addAction("Save...");
    QAction* screenshotact = fileMenu->addAction("Screenshot...");
    fileMenu->addSeparator();
    QAction* exitact = fileMenu->addAction("Exit");

    connect(openact, &QAction::triggered, this, &OSGWindow::onOpen);
    connect(saveact, &QAction::triggered, this, &OSGWindow::onSave);
    connect(screenshotact, &QAction::triggered, this, &OSGWindow::onTakeScreenshot);
    connect(exitact, &QAction::triggered, this, &OSGWindow::onExit);

    setMenuBar(menuBar);
}


OSGWindow::~OSGWindow()
{}


void OSGWindow::setOSGWidget( OSGWidget* osgwidget )
{
     osgwidget_ = osgwidget;
}


void OSGWindow::onOpen() {
    const QString path = QFileDialog::getOpenFileName(this, "Open File");
    if (!path.isEmpty()) {
        QMessageBox::information(this, "File Opened", "You selected:\n" + path);
        // TODO: load file into OSGWidget
    }
}


void OSGWindow::onSave() {
    const QString path = QFileDialog::getSaveFileName(this, "Save File");
    if (!path.isEmpty()) {
        QMessageBox::information(this, "File Saved", "Saved to:\n" + path);
        // TODO: save OSG scene
    }
}


void OSGWindow::onTakeScreenshot() {
    if ( !osgwidget_ ) {
        QMessageBox::critical(this, "Error", "No viewer available");
	return;
    }

    osgwidget_->doScreenShot( 72 );
}


void OSGWindow::onExit() {
    close(); // closes the window
}


// OSGWidget

OSGWidget::OSGWidget( QWidget* parent )
    : QOpenGLWidget(parent)
{
    setMinimumSize(600, 400);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setUpdateBehavior(QOpenGLWidget::PartialUpdate);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);

    viewer_ = new osgViewer::Viewer;
    viewer_->setThreadingModel(osgViewer::Viewer::SingleThreaded);
//    viewer_->setThreadingModel(osgViewer::Viewer::DrawThreadPerContext);

    rendertimer_ = new QTimer(this);
    connect(rendertimer_, &QTimer::timeout, this, [&]() {
	update();
    });
    rendertimer_->start(16);
}


OSGWidget::~OSGWidget()
{
    OSGWindow* osgwin = dynamic_cast<OSGWindow*>( QApplication::activeWindow() );
    if ( osgwin )
	osgwin->setOSGWidget( nullptr );
}


void OSGWidget::initializeGL() {
    // Connect OSG graphics context to Qt's native window
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = 0;
    traits->y = 0;
    traits->width = this->width();
    traits->height = this->height();
    traits->windowDecoration = false;
    traits->doubleBuffer = true;
    traits->sharedContext = nullptr;

    gw_ = new osgViewer::GraphicsWindowEmbedded(traits->x, traits->y, traits->width, traits->height);

    osg::ref_ptr<osg::Camera> camera = viewer_->getCamera();
    camera->setGraphicsContext(gw_.get());
    camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));;
    camera->setProjectionMatrixAsPerspective(30., double(width()) / height(), 1., 10000.);
    camera->setClearColor(osg::Vec4(0.2f, 0.2f, 0.2f, 1.f)); // Nice neutral gray

    // Build a scene with a cube and a line
    root_ = new osg::Group;
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;

    // Cube
    osg::ref_ptr<osg::Box> cube = new osg::Box(osg::Vec3(0.f, 0.f, 0.f), 1.f);
    osg::ref_ptr<osg::ShapeDrawable> cubeDrawable = new osg::ShapeDrawable(cube.get());
    geode->addDrawable(cubeDrawable.get());

    // Line
    osg::ref_ptr<osg::Geometry> line = new osg::Geometry;

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->push_back(osg::Vec3(-2.f, 0.f, 0.f));
    vertices->push_back(osg::Vec3(2.f, 0.f, 0.f));
    line->setVertexArray(vertices.get());
    line->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, 2));

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.f, 0.f, 0.f, 1.f)); // red
    line->setColorArray(colors.get(), osg::Array::BIND_OVERALL);

    line->getOrCreateStateSet()->setAttribute(new osg::LineWidth(3.0f), osg::StateAttribute::ON);
    geode->addDrawable(line.get());

    // Final scene setup
    root_->addChild(geode.get());

    viewer_->setSceneData(root_.get());
    viewer_->setCameraManipulator(new osgGA::TrackballManipulator());
}


static bool matricesAreClose(const osg::Matrix& m1, const osg::Matrix& m2, double epsilon = 0.0001) {
    for (int i = 0; i < 16; ++i) {
        if (std::abs(m1.ptr()[i] - m2.ptr()[i]) > epsilon) return false;
    }
    return true;
}


void OSGWidget::paintGL()
{
    if ( !viewer_.valid() )
	return;

    viewer_->frame();
    const osg::Matrix currentviewmatrix = viewer_->getCameraManipulator()->getMatrix();
    if ( matricesAreClose(currentviewmatrix,lastviewmatrix_) ) {
        stillframecount_++;
        if (stillframecount_ >= maxstillframes_) {
            rendertimer_->stop();
        }
    } else {
        stillframecount_ = 0;
        lastviewmatrix_ = currentviewmatrix;
	if (!rendertimer_->isActive()) {
            rendertimer_->start(16);
        }
    }
}


void OSGWidget::resizeGL(int w, int h)
{
    if ( gw_.valid() ) {
	gw_->resized(x(), y(), w, h);
	gw_->getEventQueue()->windowResize(x(), y(), w, h);
    }

    if ( viewer_.valid() ) {
	viewer_->getCamera()->setViewport(new osg::Viewport(0, 0, w, h));
	viewer_->getCamera()->setProjectionMatrixAsPerspective(30., static_cast<double>(w) / h, 1., 10000.);
    }
}


void OSGWidget::mousePressEvent(QMouseEvent* event) {
    const Qt::MouseButton qtbut = event->button();
    if ( qtbut != Qt::LeftButton && qtbut != Qt::MiddleButton && qtbut != Qt::RightButton )
	return;

    const QPointF pos = event->position();
    const osgGA::GUIEventAdapter::MouseButtonMask osgbut = mapQtMouseButton(qtbut);
    gw_->getEventQueue()->mouseButtonPress(pos.x(), pos.y(), osgbut);
    update();
}


void OSGWidget::mouseReleaseEvent(QMouseEvent* event) {
    const Qt::MouseButton qtbut = event->button();
    if ( qtbut != Qt::LeftButton && qtbut != Qt::MiddleButton && qtbut != Qt::RightButton )
	return;

    const QPointF pos = event->position();
    const osgGA::GUIEventAdapter::MouseButtonMask osgbut = mapQtMouseButton(qtbut);
    gw_->getEventQueue()->mouseButtonRelease(pos.x(), pos.y(), osgbut);
    if ( !rendertimer_->isActive() )
	rendertimer_->start(16);
}


void OSGWidget::mouseMoveEvent(QMouseEvent* event) {
    const QPointF pos = event->position();
    gw_->getEventQueue()->mouseMotion(pos.x(), pos.y());
    if ( !rendertimer_->isActive() )
	rendertimer_->start(16);
}


void OSGWidget::wheelEvent(QWheelEvent* event) {
    const float delta = static_cast<float>(event->angleDelta().y()) / 120.0f; // Steps
    if (delta > 0)
	gw_->getEventQueue()->mouseScroll(osgGA::GUIEventAdapter::SCROLL_UP);
    else
	gw_->getEventQueue()->mouseScroll(osgGA::GUIEventAdapter::SCROLL_DOWN);

    update();
}


void OSGWidget::keyPressEvent(QKeyEvent* event) {
    gw_->getEventQueue()->keyPress(mapQtKey(event));
    update();
}


void OSGWidget::keyReleaseEvent(QKeyEvent* event) {
    gw_->getEventQueue()->keyRelease(mapQtKey(event));
    update();
}


osgGA::GUIEventAdapter::MouseButtonMask OSGWidget::mapQtMouseButton(Qt::MouseButton button) {
    switch (button) {
	case Qt::LeftButton: return osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON;
	case Qt::MiddleButton: return osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON;
	case Qt::RightButton: return osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON;
	default: return osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON;
    }
}


int OSGWidget::mapQtKey(QKeyEvent* event) {
    return event->key();
}


void OSGWidget::doScreenShot( int res ) const
{
    osg::ref_ptr<osg::Camera> cameraRTT = new osg::Camera;
    cameraRTT->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    cameraRTT->setRenderOrder(osg::Camera::POST_RENDER);
    cameraRTT->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    cameraRTT->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
//    cameraRTT->setClearColor(osg::Vec4(1.0, 1.0, 1.0, 1.0)); // white background

    int screenshotWidth = 1920;
    int screenshotHeight = 1080;
    cameraRTT->setViewport(0, 0, screenshotWidth, screenshotHeight);
    cameraRTT->setProjectionMatrixAsPerspective(30.0f,
	    static_cast<double>(screenshotWidth) / screenshotHeight,
	    1.0f, 10000.0f);

    osg::ref_ptr<osg::Image> image = new osg::Image;
    cameraRTT->attach(osg::Camera::COLOR_BUFFER, image.get());
    cameraRTT->addChild(root_.get());
/* TODO: make it not crash
    osgViewer::Viewer viewer;
    viewer.setSceneData(cameraRTT.get());
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.frame();
    if (image.valid() && image->data()) {
	osgDB::writeImageFile(*image, "/tmp/screenshot.png");
    }
 */
}


// main app

int main(int argc, char** argv) {
    // Set up compatibility OpenGL profile
    QSurfaceFormat fmt;
    fmt.setVersion(2, 0);
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    fmt.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);

    OSGWindow window;
    window.setWindowTitle("Minimal Qt6 + OSG 3.6.5 Embedded Example");

    auto* mdiarea = new QMdiArea;
    window.setCentralWidget(mdiarea);

    auto* osgwidget = new OSGWidget;
    auto* container = new QWidget;

    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(osgwidget);

    container->setLayout(layout);

    auto* subwindow = new QMdiSubWindow;
    subwindow->setWindowTitle("3D View");
    subwindow->setWidget(container);
    subwindow->setAttribute(Qt::WA_DeleteOnClose);
    mdiarea->addSubWindow(subwindow);
    subwindow->showMaximized();

    window.setOSGWidget( osgwidget );
    window.resize( 800, 600 );
    window.show();

    return app.exec();
}
