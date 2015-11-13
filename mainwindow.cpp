#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <setupapi.h>
#include <Cfgmgr32.h>
#include <Regstr.h>

#include <QDebug>
#include <QToolButton>
#include <QLabel>
#include <QMainWindow>
#include <QPalette>
#include <QFile>

class MainWindowCallback
{
public:
    static MainWindow* ptr;
    static void my_callback() {
        ptr->re_enum_need();
    }
};
MainWindow* MainWindowCallback::ptr = NULL;

void MainWindow::EnumerateDeviceTree()
{
    DeviceInfoSet = SetupDiGetClassDevs(NULL, 0, 0, SetupDiGetClassDevsFlags);
    SP_DEVINFO_DATA DeviceInfoData;
    ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    wchar_t PropertyBufferW[128] = { 0 };
    DWORD RequiredSize = 0;

    wchar_t GUID_w[128] = { 0 };
    DWORD GUID_w_size = 0;

    DWORD i = 0;
    while(SetupDiEnumDeviceInfo(DeviceInfoSet, i, &DeviceInfoData)) {
        RequiredSize = 256;
        if(CM_Get_DevNode_Registry_PropertyW(DeviceInfoData.DevInst, CM_DRP_FRIENDLYNAME, NULL, PropertyBufferW, &RequiredSize, 0) != CR_SUCCESS || RequiredSize == 0) {
            RequiredSize = 256;
            if(CM_Get_DevNode_Registry_PropertyW(DeviceInfoData.DevInst, CM_DRP_DEVICEDESC, NULL, PropertyBufferW, &RequiredSize, 0) != CR_SUCCESS || RequiredSize == 0) {
                 i++;
                 continue;
            }
        }
        if(CM_Get_DevNode_Registry_PropertyW(DeviceInfoData.DevInst, CM_DRP_CLASSGUID, NULL, GUID_w, &GUID_w_size, 0) == CR_SUCCESS && GUID_w_size != 0) {
            GUID_w_size = 128;
            QString GUID_s = QString::fromWCharArray(GUID_w, GUID_w_size/2 - 1);
            DeviceInfo temp;
            temp.DeviceDescription = QString::fromWCharArray(PropertyBufferW, RequiredSize/2 - 1);
            memcpy(&(temp.DeviceInfoData), &DeviceInfoData, DeviceInfoData.cbSize);

            RequiredSize = 256;
            CM_Get_DevNode_Registry_PropertyW(DeviceInfoData.DevInst, CM_DRP_CLASS, NULL, PropertyBufferW, &RequiredSize, 0);
            temp.DeviceClass = QString::fromWCharArray(PropertyBufferW, RequiredSize/2 - 1);

            if(DeviceTree.values(GUID_s).isEmpty()) {
                GUIDList.append(GUID_s);
            }
            DWORD flags = 0;
            DWORD flags_size = 4;
            if(CM_Get_DevNode_Registry_PropertyW(DeviceInfoData.DevInst, CM_DRP_CONFIGFLAGS, NULL, &flags, &flags_size, 0) == CR_SUCCESS) {
                temp.isDisabled = flags & CONFIGFLAG_DISABLED;
            }

            ULONG  pulStatus = 0;
            ULONG  pulProblemNumber = 0;
            CONFIGRET CM_RET = CR_SUCCESS;
            CM_RET = CM_Get_DevNode_Status(&pulStatus, &pulProblemNumber, DeviceInfoData.DevInst, 0);
            if(CM_RET == CR_NO_SUCH_DEVINST) {
                temp.isPlugged = false;
            }
            if(CM_RET == CR_SUCCESS) {
                temp.isPlugged = true;
            }
            if(pulStatus & DN_HAS_PROBLEM) {
                temp.isGood = false;
            } else {
                temp.isGood = true;
            }
            if(pulStatus & DN_DISABLEABLE){
                temp.isDisableable = true;
            } else {
                temp.isDisableable = false;
            }
            DeviceTree.insert(GUID_s, temp);
        }
        i++;
    }
}
void MainWindow::UpdateTreeView()
{
    DWORD RequiredSize = 0;
    wchar_t PropertyBufferW[256] = { 0 };
    for(int i = 0; i < GUIDList.size(); i++) {
        QList<DeviceInfo> dev_info_list = DeviceTree.values(GUIDList.at(i));
        RequiredSize = 256;
        QTreeWidgetItem *ClassSubTree = new QTreeWidgetItem(ui->treeWidget);

        if(SetupDiGetClassDescriptionW(&(dev_info_list.at(0).DeviceInfoData.ClassGuid), PropertyBufferW,  RequiredSize,  &RequiredSize)) {
            ClassSubTree->setText(0, QString::fromWCharArray(PropertyBufferW, RequiredSize - 1));

        }

        for(int j = 0; j < dev_info_list.size(); j++) {
            QTreeWidgetItem *ClassSubTreeChild = new QTreeWidgetItem();
            QVariant data = QVariant::fromValue(dev_info_list.at(j));
            ClassSubTreeChild->setData(0, Qt::UserRole, data);
            ClassSubTreeChild->setText(0, dev_info_list.at(j).DeviceDescription);

            if(dev_info_list.at(j).isGood){
                ClassSubTreeChild->setIcon(0, QIcon(":/state/icons/state/isGood.svg"));
            } else {
                 ClassSubTreeChild->setIcon(0, QIcon(":/state/icons/state/hasProblem.svg"));
            }

            if(dev_info_list.at(j).isDisabled) {
                //ClassSubTreeChild->setForeground(0, QBrush(Qt::red));
                ClassSubTreeChild->setIcon(0, QIcon(":/state/icons/state/isDisabled.svg"));
            }

            if(!dev_info_list.at(j).isPlugged) {
                ClassSubTreeChild->setForeground(0, QBrush(Qt::gray));
                ClassSubTreeChild->setIcon(0, QIcon(":/state/icons/state/isNotPlugged.svg").pixmap(16, 16, QIcon::Disabled));
            }

            ClassSubTree->addChild(ClassSubTreeChild);

            if(QFile::exists(QString(":/classes/icons/classes/" + dev_info_list.first().DeviceClass + ".svg"))) {
                ClassSubTree->setIcon(0, QIcon(QString(":/classes/icons/classes/" + dev_info_list.first().DeviceClass + ".svg")));
            } else {
                ClassSubTree->setIcon(0, QIcon(QString(":/classes/icons/classes/NO_CLASS.svg")));
            }
        }
    }
}
void MainWindow::on_reEnumToolButtonClicked()
{
    deviceEnableToolButtonAction->setVisible(false);
    deviceDisableToolButtonAction->setVisible(false);
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    ui->treeWidget->clear();
    DeviceTree.clear();
    GUIDList.clear();
    EnumerateDeviceTree();
    UpdateTreeView();
}
void MainWindow::on_changeFlagsToolButtonClicked()
{
    if(SetupDiGetClassDevsFlags == (DIGCF_PRESENT | DIGCF_ALLCLASSES)) {
        SetupDiGetClassDevsFlags = DIGCF_ALLCLASSES;
        changeFlagsToolButton->setIcon(QIcon(":/ui/icons/ui/eye.svg"));
    } else {
        SetupDiGetClassDevsFlags = DIGCF_PRESENT | DIGCF_ALLCLASSES;
        changeFlagsToolButton->setIcon(QIcon(":/ui/icons/ui/eye-with-line.svg"));
    }
    on_reEnumToolButtonClicked();
}
void MainWindow::on_deviceEnableToolButtonClicked()
{
    QVariant a = ui->treeWidget->currentItem()->data(0, Qt::UserRole);
    DeviceInfo dev_info = a.value<DeviceInfo>();
    if(dev_info.DeviceDescription.isEmpty() || !dev_info.isDisableable) {
        deviceEnableToolButtonAction->setVisible(false);
        deviceDisableToolButtonAction->setVisible(false);
        return;
    }

    if(!dev_info.isDisabled){
        deviceEnableToolButtonAction->setVisible(false);
        deviceDisableToolButtonAction->setVisible(true);
        return;
    }
    /*if(CM_Enable_DevNode(dev_info.DeviceInfoData.DevInst, 0) == CR_SUCCESS) {
        qDebug() << "enable" << dev_info.DeviceDescription;
    } else {
        qDebug() << "failed enable" << dev_info.DeviceDescription;
    }*/

    SP_PROPCHANGE_PARAMS ClassInstallParams;
    ZeroMemory(&ClassInstallParams, sizeof(SP_PROPCHANGE_PARAMS));
    ClassInstallParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    ClassInstallParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    ClassInstallParams.HwProfile = 0;
    ClassInstallParams.Scope = DICS_FLAG_GLOBAL;
    ClassInstallParams.StateChange = DICS_ENABLE;

    if(SetupDiSetClassInstallParams(DeviceInfoSet, &(dev_info.DeviceInfoData), (SP_CLASSINSTALL_HEADER *)&ClassInstallParams, sizeof(ClassInstallParams))) {
        if(SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, DeviceInfoSet, &(dev_info.DeviceInfoData))) {
            qDebug() << "enable" << dev_info.DeviceDescription;
        } else {
            qDebug() << "failed SetupDiCallClassInstaller" << dev_info.DeviceDescription;
        }
    } else {
        qDebug() << "failed SetupDiSetClassInstallParams" << dev_info.DeviceDescription;
    }
}
void MainWindow::on_deviceDisableToolButtonClicked()
{
    QVariant a = ui->treeWidget->currentItem()->data(0, Qt::UserRole);
    DeviceInfo dev_info = a.value<DeviceInfo>();
    if(dev_info.DeviceDescription.isEmpty() || !dev_info.isDisableable) {
        deviceEnableToolButtonAction->setVisible(false);
        deviceDisableToolButtonAction->setVisible(false);
        return;
    }

    if(dev_info.isDisabled){
        deviceDisableToolButtonAction->setVisible(false);
        deviceEnableToolButtonAction->setVisible(true);
        return;
    }

    SP_PROPCHANGE_PARAMS ClassInstallParams;
    ZeroMemory(&ClassInstallParams, sizeof(SP_PROPCHANGE_PARAMS));
    ClassInstallParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    ClassInstallParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    ClassInstallParams.HwProfile = 0;
    ClassInstallParams.Scope = DICS_FLAG_GLOBAL;
    ClassInstallParams.StateChange = DICS_DISABLE;

    if(SetupDiSetClassInstallParams(DeviceInfoSet, &(dev_info.DeviceInfoData), (SP_CLASSINSTALL_HEADER *)&ClassInstallParams, sizeof(ClassInstallParams))) {
        if(SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, DeviceInfoSet, &(dev_info.DeviceInfoData))) {
            qDebug() << "disable" << dev_info.DeviceDescription;
        } else {
            qDebug() << "failed SetupDiCallClassInstaller" << dev_info.DeviceDescription;
        }
    } else {
        qDebug() << "failed SetupDiSetClassInstallParams" << dev_info.DeviceDescription;
    }
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->treeWidget->setColumnCount(1);
    ui->treeWidget->setSortingEnabled(true);
    this->setWindowIcon(QIcon(":/ui/icons/ui/laptop.svg"));


    reEnumToolButton = new QToolButton();
    {
        reEnumToolButton->setIcon(QIcon(":/ui/icons/ui/re_enum.svg"));
        connect(reEnumToolButton, SIGNAL(clicked(bool)), this, SLOT(on_reEnumToolButtonClicked()));
        ui->toolBar->addWidget(reEnumToolButton);

    }

    changeFlagsToolButton = new QToolButton;
    {
        changeFlagsToolButton->setIcon(QIcon(":/ui/icons/ui/eye-with-line.svg"));
        connect(changeFlagsToolButton, SIGNAL(clicked(bool)), this, SLOT(on_changeFlagsToolButtonClicked()));
        ui->toolBar->addWidget(changeFlagsToolButton);
    }

    deviceEnableToolButton = new QToolButton;
    {
        deviceEnableToolButton->setIcon(QIcon(":/ui/icons/ui/install.svg"));
        connect(deviceEnableToolButton, SIGNAL(clicked(bool)), this, SLOT(on_deviceEnableToolButtonClicked()));
        deviceEnableToolButtonAction = ui->toolBar->addWidget(deviceEnableToolButton);
        deviceEnableToolButtonAction->setVisible(false);
    }

    deviceDisableToolButton = new QToolButton;
    {
        deviceDisableToolButton->setIcon(QIcon(":/ui/icons/ui/uninstall.svg"));
        connect(deviceDisableToolButton, SIGNAL(clicked(bool)), this, SLOT(on_deviceDisableToolButtonClicked()));
        deviceDisableToolButtonAction = ui->toolBar->addWidget(deviceDisableToolButton);
        deviceDisableToolButtonAction->setVisible(false);
    }

    EnumerateDeviceTree();
    UpdateTreeView();

    HMODULE hLib;
    hLib = LoadLibraryW((LPCWSTR)L"./CM_Notific_dll.dll");
    if(hLib != NULL) {
        bool (*Init_DeviceInterface_Notification)(wchar_t *);
        (FARPROC &)(Init_DeviceInterface_Notification) = GetProcAddress(hLib, "Init_DeviceInterface_Notification");
        if(Init_DeviceInterface_Notification((wchar_t*)L"C:/Windows/System32/downlevel/API-MS-Win-devices-config-L1-1-1.dll")){
            bool (*Register_DeviceInterface_Notification)(void(*)(void));
            (FARPROC &)(Register_DeviceInterface_Notification) = GetProcAddress(hLib, "Register_DeviceInterface_Notification");
            MainWindowCallback::ptr = this;
            connect(this, SIGNAL(re_enum_need()), this, SLOT(on_reEnumToolButtonClicked()));
            Register_DeviceInterface_Notification((void(*)())&MainWindowCallback::my_callback);
        } else {
            qDebug() << "Can't load API-MS-Win-devices-config-L1-1-1.dll. Device tree autoupdate disabled";
        }

    }else{
        qDebug() << "Can't load CM_Notific_dll.dll. Device tree autoupdate disabled";
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QVariant a = item->data(column, Qt::UserRole);
    DeviceInfo dev_info = a.value<DeviceInfo>();
    if(dev_info.DeviceDescription.isEmpty()) {
        return;
    }

    QLabel *DevDescLabel = new QLabel();
    wchar_t PropertyBufferW[128] = { 0 };

    DWORD RequiredSize = 256;
    if(CM_Get_DevNode_Registry_PropertyW(dev_info.DeviceInfoData.DevInst, CM_DRP_CLASS, NULL, PropertyBufferW, &RequiredSize, 0) == CR_SUCCESS && RequiredSize != 0) {
        DevDescLabel->setText(DevDescLabel->text() + "Class: \t\t\t" + QString::fromWCharArray(PropertyBufferW, RequiredSize/2 - 1));
    }

    RequiredSize = 256;
    if(CM_Get_DevNode_Registry_PropertyW(dev_info.DeviceInfoData.DevInst, CM_DRP_CLASSGUID, NULL, PropertyBufferW, &RequiredSize, 0) == CR_SUCCESS && RequiredSize != 0) {
        DevDescLabel->setText(DevDescLabel->text() + "\nClass GUID: \t\t" + QString::fromWCharArray(PropertyBufferW, RequiredSize/2 - 1));
    }

    RequiredSize = 256;
    if(CM_Get_DevNode_Registry_PropertyW(dev_info.DeviceInfoData.DevInst, CM_DRP_ENUMERATOR_NAME, NULL, PropertyBufferW, &RequiredSize, 0) == CR_SUCCESS && RequiredSize != 0) {
        DevDescLabel->setText(DevDescLabel->text() + "\nEnumerator: \t\t" + QString::fromWCharArray(PropertyBufferW, RequiredSize/2 - 1));
    }

    RequiredSize = 256;
    if(CM_Get_DevNode_Registry_PropertyW(dev_info.DeviceInfoData.DevInst, CM_DRP_LOCATION_INFORMATION, NULL, PropertyBufferW, &RequiredSize, 0) == CR_SUCCESS && RequiredSize != 0) {
        DevDescLabel->setText(DevDescLabel->text() + "\nLocation: \t\t\t" + QString::fromWCharArray(PropertyBufferW, RequiredSize/2 - 1));
    }

    RequiredSize = 256;
    if(CM_Get_DevNode_Registry_PropertyW(dev_info.DeviceInfoData.DevInst, CM_DRP_MFG, NULL, PropertyBufferW, &RequiredSize, 0) == CR_SUCCESS && RequiredSize != 0) {
        DevDescLabel->setText(DevDescLabel->text() + "\nManufacturer: \t\t" + QString::fromWCharArray(PropertyBufferW, RequiredSize/2 - 1));
    }

    RequiredSize = 256;
    if(CM_Get_DevNode_Registry_PropertyW(dev_info.DeviceInfoData.DevInst, CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME, NULL, PropertyBufferW, &RequiredSize, 0) == CR_SUCCESS && RequiredSize != 0) {
        DevDescLabel->setText(DevDescLabel->text() + "\nPhysical Object Name: \t" + QString::fromWCharArray(PropertyBufferW, RequiredSize/2 - 1));
    }

    RequiredSize = 256;
    if(CM_Get_DevNode_Registry_PropertyW(dev_info.DeviceInfoData.DevInst, CM_DRP_SECURITY_SDS, NULL, PropertyBufferW, &RequiredSize, 0) == CR_SUCCESS && RequiredSize != 0) {
        DevDescLabel->setText(DevDescLabel->text() + "\nSecurity descriptor: \t" + QString::fromWCharArray(PropertyBufferW, RequiredSize/2 - 1));
    }

    RequiredSize = 256;
    if(CM_Get_DevNode_Registry_PropertyW(dev_info.DeviceInfoData.DevInst, CM_DRP_SERVICE, NULL, PropertyBufferW, &RequiredSize, 0) == CR_SUCCESS && RequiredSize != 0) {
        DevDescLabel->setText(DevDescLabel->text() + "\nService: \t\t\t" + QString::fromWCharArray(PropertyBufferW, RequiredSize/2 - 1));
    }

    RequiredSize = 256;
    if(CM_Get_DevNode_Registry_PropertyW(dev_info.DeviceInfoData.DevInst, CM_DRP_FRIENDLYNAME, NULL, PropertyBufferW, &RequiredSize, 0) == CR_SUCCESS && RequiredSize != 0) {
        DevDescLabel->setText(DevDescLabel->text() + "\nDevice Name: \t\t" + QString::fromWCharArray(PropertyBufferW, RequiredSize/2 - 1));
    }

    RequiredSize = 256;
    if(CM_Get_DevNode_Registry_PropertyW(dev_info.DeviceInfoData.DevInst, CM_DRP_DEVICEDESC, NULL, PropertyBufferW, &RequiredSize, 0) == CR_SUCCESS && RequiredSize != 0) {
        DevDescLabel->setText(DevDescLabel->text() + "\nDevice Description: \t\t" + QString::fromWCharArray(PropertyBufferW, RequiredSize/2 - 1));
    }


    QWidget *w = new QWidget();
    {
        w->setMinimumSize(0, 0);
        w->setWindowFlags(Qt::WindowCloseButtonHint);
        w->setPalette(((QMainWindow*)(ui->centralWidget->parent()))->palette());
        w->setWindowTitle(dev_info.DeviceDescription);
        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(DevDescLabel);
        w->setLayout(layout);
        w->layout()->addWidget(DevDescLabel);
        w->setWindowIcon(QIcon(":/ui/icons/ui/info-with-circle.svg"));
    }
    w->show();
}

void MainWindow::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    QVariant a = item->data(column, Qt::UserRole);
    DeviceInfo dev_info = a.value<DeviceInfo>();
    if(dev_info.DeviceDescription.isEmpty()) {
        deviceEnableToolButtonAction->setVisible(false);
        deviceDisableToolButtonAction->setVisible(false);
        return;
    }

    if(dev_info.isDisableable && dev_info.isDisabled){
        deviceEnableToolButtonAction->setVisible(true);
    } else {
        deviceEnableToolButtonAction->setVisible(false);
    }

    if(dev_info.isDisableable && !dev_info.isDisabled) {
        deviceDisableToolButtonAction->setVisible(true);
    } else {
        deviceDisableToolButtonAction->setVisible(false);
    }
}
