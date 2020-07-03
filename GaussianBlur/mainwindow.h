#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QImage image;
    QList<float> weights;

    bool loadFile(const QString &fileName);
    void setImage(const QImage &newImage);
    bool saveFile(const QString &fileName);
    void Get2DGaussianWeights(int kernelSize, QList<float> &weightsList);
    void Do2DGaussianBlur(int kernelSize);
    void Do1DGaussianBlur(int kernelSize);
    void Get1DGaussianWeights(int kernelSize, QList<float> &weightsList);
private slots:
        void on_actionOpenFile_triggered();
        void on_actionSaveFile_triggered();
        void on_actionBlurImage_triggered();
};

#endif // MAINWINDOW_H
