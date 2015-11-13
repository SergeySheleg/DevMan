#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>

#include <windows.h>
#include <setupapi.h>
#include <QToolButton>
#include <QAction>
#include <QDebug>


namespace Ui
{
    class MainWindow;
}

class DeviceInfo
{
public:
    QString DeviceDescription;
    QString DeviceClass;
    SP_DEVINFO_DATA DeviceInfoData;

    bool isDisabled;
    bool isPlugged;
    bool isDisableable;
    bool isGood;
};
Q_DECLARE_METATYPE(DeviceInfo)


class MainWindow : public QMainWindow
{
    Q_OBJECT

signals:
    void re_enum_need();

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);

    void on_reEnumToolButtonClicked();
    void on_changeFlagsToolButtonClicked();
    void on_deviceEnableToolButtonClicked();
    void on_deviceDisableToolButtonClicked();

private:
    Ui::MainWindow *ui;
    QToolButton *reEnumToolButton;
    QToolButton *changeFlagsToolButton;
    QToolButton *deviceEnableToolButton;
    QToolButton *deviceDisableToolButton;
    QAction *deviceEnableToolButtonAction;
    QAction *deviceDisableToolButtonAction;

    DWORD SetupDiGetClassDevsFlags = DIGCF_PRESENT | DIGCF_ALLCLASSES;
    HDEVINFO DeviceInfoSet;

    QMultiMap<QString, DeviceInfo> DeviceTree;
    QList<QString> GUIDList;

private:
    void EnumerateDeviceTree();
    void UpdateTreeView();
};

#endif // MAINWINDOW_H

