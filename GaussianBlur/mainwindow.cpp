#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QStandardPaths>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QMessageBox>
#include <QtMath>
using namespace std;

static void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode)
{
    static bool firstDialog = true;

    if (firstDialog) {
        firstDialog = false;
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    }

    if (acceptMode == QFileDialog::AcceptSave)
        dialog.setDefaultSuffix("jpg");
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    image(":/res/default.jpg")
{
    ui->setupUi(this);
    setImage(image);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::loadFile(const QString &fileName)
{
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    if (newImage.isNull()) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot load %1: %2")
                                 .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }

    setImage(newImage);

    setWindowFilePath(fileName);

    const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
        .arg(QDir::toNativeSeparators(fileName)).arg(image.width()).arg(image.height()).arg(image.depth());
    ui->statusBar->showMessage(message);
    return true;
}

bool MainWindow::saveFile(const QString &fileName)
{
    QImageWriter writer(fileName);

    if (!writer.write(image)) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot write %1: %2")
                                 .arg(QDir::toNativeSeparators(fileName)), writer.errorString());
        return false;
    }
    const QString message = tr("Wrote \"%1\"").arg(QDir::toNativeSeparators(fileName));
    statusBar()->showMessage(message);
    return true;
}

void MainWindow::setImage(const QImage &newImage)
{
    image = newImage;
    ui->label->setPixmap(QPixmap::fromImage(image));
}

void MainWindow::on_actionOpenFile_triggered()
{
    qDebug() << "hello world" << endl;

    QFileDialog dialog(this, tr("Open File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);

    while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().first())) {}
}

void MainWindow::on_actionSaveFile_triggered()
{
    QFileDialog dialog(this, tr("Save File As"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptSave);

    while (dialog.exec() == QDialog::Accepted && !saveFile(dialog.selectedFiles().first())) {}
}

static float CalcGaussianValue(float x)
{
    float segma = 1.0f;
    float segma2 = segma*segma;
    float mu = 0.0f;
    float xMinusMu2 = (x-mu)*(x-mu);
    return 1.0f / (qSqrt(2*M_PI)*segma) * qExp(-xMinusMu2/(2*segma2));
}

void MainWindow::Get2DGaussianWeights(int kernelSize, QList<float>& weightsList)
{
    weightsList.clear();
    if(kernelSize%2 == 0) return;

    float sum = 0;
    float gaussianV = 0;
    int halfKernelSize = kernelSize/2;
    for(int x=-halfKernelSize; x<=halfKernelSize; x++)
    {
        for(int y=-halfKernelSize; y<=halfKernelSize; y++)
        {
            gaussianV = CalcGaussianValue(qSqrt(x*x+y*y));
            sum += gaussianV;
            weightsList.append(gaussianV);
        }
    }

    // normalize
    for(int i=0; i<weightsList.count(); i++)
    {
        weightsList[i] /= sum;
    }
}

void MainWindow::Get1DGaussianWeights(int kernelSize, QList<float>& weightsList)
{
    Get2DGaussianWeights(kernelSize, weightsList);
    if(weightsList.empty()) return;

    QList<float> newWeightList;
    for(int i=0; i<kernelSize; i++)
    {
        float w = weightsList[i*kernelSize+i];
        newWeightList.append(qSqrt(w));
    }
    weightsList = newWeightList;
}

void MainWindow::Do2DGaussianBlur(int kernelSize)
{
    Get2DGaussianWeights(kernelSize, weights);
    int halfKernelSize = kernelSize/2;
    int pixelX;
    int pixelY;

    QImage tmpImage(image);
    for(int r=0; r<image.width(); r++)
    {
        for(int c=0; c<image.height(); c++)
        {
            double sumColorR = 0;
            double sumColorG = 0;
            double sumColorB = 0;
            for(int i=-halfKernelSize; i<=halfKernelSize; i++)
            {
                for(int j=-halfKernelSize; j<=halfKernelSize; j++)
                {
                    pixelX = qAbs(r+i);
                    pixelY = qAbs(c+j);
                    if(pixelX>=image.width())
                    {
                        pixelX -= (pixelX-image.width()+1);
                    }
                    if(pixelY>=image.height())
                    {
                        pixelY -= (pixelY-image.height()+1);
                    }
                    QColor tmpColor = image.pixelColor(pixelX, pixelY);
                    int weightIdx = (i+halfKernelSize) * kernelSize + (j+halfKernelSize);
                    int r = tmpColor.red();
                    int g = tmpColor.green();
                    int b = tmpColor.blue();
                    sumColorR += r * weights.at(weightIdx);
                    sumColorG += g * weights.at(weightIdx);
                    sumColorB += b * weights.at(weightIdx);
                }
            }
            tmpImage.setPixelColor(r,c,QColor(sumColorR,sumColorG,sumColorB));
        }
    }
    image = tmpImage;
}

void MainWindow::Do1DGaussianBlur(int kernelSize)
{
    Get1DGaussianWeights(kernelSize, weights);
    int halfKernelSize = kernelSize/2;
    int pixelX;
    int pixelY;

    QImage tmpImage(image);
    for(int r=0; r<image.width(); r++)
    {
        for(int c=0; c<image.height(); c++)
        {
            double sumColorR = 0;
            double sumColorG = 0;
            double sumColorB = 0;
            // horizontal blur
            for(int i=-halfKernelSize; i<=halfKernelSize; i++)
            {
                pixelX = qAbs(r+i);
                pixelY = c;
                if(pixelX>=image.width())
                {
                    pixelX -= (pixelX-image.width()+1);
                }

                QColor tmpColor = image.pixelColor(pixelX, pixelY);
                int weightIdx = i+halfKernelSize;
                int r = tmpColor.red();
                int g = tmpColor.green();
                int b = tmpColor.blue();
                sumColorR += r * weights.at(weightIdx);
                sumColorG += g * weights.at(weightIdx);
                sumColorB += b * weights.at(weightIdx);
            }
            tmpImage.setPixelColor(r,c,QColor(sumColorR,sumColorG,sumColorB));
        }
    }

    for(int r=0; r<tmpImage.width(); r++)
    {
        for(int c=0; c<tmpImage.height(); c++)
        {
            double sumColorR = 0;
            double sumColorG = 0;
            double sumColorB = 0;
            // vertical blur
            for(int i=-halfKernelSize; i<=halfKernelSize; i++)
            {
                pixelX = r;
                pixelY = qAbs(c+i);
                if(pixelY>=tmpImage.height())
                {
                    pixelY -= (pixelY-tmpImage.height()+1);
                }

                QColor tmpColor = tmpImage.pixelColor(pixelX, pixelY);
                int weightIdx = i+halfKernelSize;
                int r = tmpColor.red();
                int g = tmpColor.green();
                int b = tmpColor.blue();
                sumColorR += r * weights.at(weightIdx);
                sumColorG += g * weights.at(weightIdx);
                sumColorB += b * weights.at(weightIdx);
            }
            image.setPixelColor(r,c,QColor(sumColorR,sumColorG,sumColorB));
        }
    }
}

void MainWindow::on_actionBlurImage_triggered()
{
    if(image.width() <1 || image.height()<1)
    {
        QMessageBox::information(this, "Warnning", "image is empty!");
        return;
    }
    int kernelSize = ui->kernelSizeSpin->value();
    if(kernelSize%2 == 0)
    {
        QMessageBox::information(this, "Warnning", "Kernel Size should be odd!");
        return;
    }

    bool checked = ui->checkBox->isChecked();
    if(checked)
    {
        Do1DGaussianBlur(kernelSize);
    }
    else
    {
        Do2DGaussianBlur(kernelSize);
    }

    setImage(image);
}
