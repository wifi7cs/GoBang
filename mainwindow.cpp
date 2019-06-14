#include "mainwindow.h"
#include "config.h"
#include <QApplication>
#include <QCloseEvent>
#include <QPainter>
#include <QTimer>
#include <QMouseEvent>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDebug>
#include <QFile>
#include <QListWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <string.h>
#include <math.h>
#include <fstream>
#include <stack>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 设置棋盘大小
    setFixedSize(kBoardMargin * 2 + kBlockSize * (kBoardSizeNum - 1), kBoardMargin * 2 + kBlockSize * (kBoardSizeNum - 1));

    // 添加“下一步”按钮
    QPushButton *nextBtn = new QPushButton(this);
    nextBtn->move(kBoardMargin-75+kBlockSize*(kBoardSizeNum-1)/2,kBoardMargin*1.15+kBlockSize*(kBoardSizeNum-1));
    nextBtn->setText(tr("下一步"));
    nextBtn->setFixedSize(150,kBoardMargin*0.8);
    nextBtn->hide();
    connect(nextBtn,SIGNAL(clicked()),this,SLOT(showNext()));

    // 开启鼠标hover功能，这两句一般要设置window的
    setMouseTracking(true);
    // centralWidget()->setMouseTracking(true);

    // 添加菜单
    QMenu *gameMenu = menuBar()->addMenu(tr("新对局"));
    QAction *actionPVP = new QAction("双人对战", this);
    connect(actionPVP, SIGNAL(triggered()), this, SLOT(initPVPGame()));
    connect(actionPVP,SIGNAL(triggered()),nextBtn,SLOT(hide()));
    gameMenu->addAction(actionPVP);

    QAction *actionPVE = new QAction("人机对战", this);
    connect(actionPVE, SIGNAL(triggered()), this, SLOT(initPVEGame()));
    connect(actionPVE,SIGNAL(triggered()),nextBtn,SLOT(hide()));
    gameMenu->addAction(actionPVE);

    QAction *actionQuit = new QAction ("退出", this);
    connect(actionQuit, SIGNAL(triggered()), this, SLOT(quitGame()));
    gameMenu->addAction(actionQuit);

    QMenu *gameTool = menuBar()->addMenu(tr("工具"));
    QAction *regretAction = new QAction("悔棋", this);
    connect(regretAction,SIGNAL(triggered()),this,SLOT(regret()));
    gameTool->addAction(regretAction);

    QAction *openAction = new QAction("打开棋谱", this);
    connect(openAction, SIGNAL(triggered()), this, SLOT(openGame()));

    gameTool->addAction(openAction);

    QMenu *gameHelp = menuBar()->addMenu(tr("帮助"));
    QAction *actionIntro = new QAction("关于", this);
    connect(actionIntro, SIGNAL(triggered()), this, SLOT(showIntro()));
    gameHelp->addAction(actionIntro);

    connect(this,SIGNAL(open()),nextBtn,SLOT(show()));
    //connect(openAction, SIGNAL(triggered()), nextBtn, SLOT(show()));
    connect(this,SIGNAL(finish()),nextBtn,SLOT(hide()));

    // 开始游戏
    initGame();
}

MainWindow::~MainWindow()
{
    if (game)
    {
        delete game;
        game = nullptr;
    }
}

void MainWindow::initGame()
{
    // 初始化游戏模型
    game = new GameModel;
    //打开后默认为双人对战模式
    initPVPGame();
}

void MainWindow::initPVPGame()
{
    game_type = PERSON;
    game->gameStatus = PLAYING;
    game->startGame(game_type);
    update();
}

void MainWindow::initPVEGame()
{
    game_type = BOT;
    game->gameStatus = PLAYING;
    game->startGame(game_type);
    update();
}

//点击退出选项关闭程序
void MainWindow::quitGame()
{
    if (!(QMessageBox::information(this, tr("退出"), tr("是否真的要退出游戏？"), tr("是"), tr("否"))))
    {
        QApplication *app;
        app->exit(0);
    }
}

//点击右上角红色X关闭程序
void MainWindow::closeEvent(QCloseEvent *event)
{
   switch( QMessageBox::information( this, tr("退出"), tr("是否真的要退出游戏？"), tr("是"), tr("否"), 0, 1 ) )
   {
   case 0:
       event->accept();
       break;
   case 1:
   default:
       event->ignore();
       break;
   }
}

void MainWindow::showIntro()
{
    QMessageBox::about(this, tr("关于GoBang"), tr("游戏名：GoBang\n\n版本号：1.0\n\n作者：吴亦飞 唐芮琪 楼琳妃\n\n版权所有 © 2019"));
}

void MainWindow::regret()
{
    game->regret();
    update();
}

void MainWindow::saveGame()
{
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("保存棋谱"),"",tr("棋谱文件 (*.chess)"));
    std::ofstream fout(fileName.toStdString());
    while(!game->posStack.empty())
    {
        fout<<game->posStack.top().first<<" "<<game->posStack.top().second<<std::endl;
        game->posStack.pop();
    }
    fout.close();
}

void MainWindow::openGame()
{
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("打开棋谱"),tr("C://"),tr("棋谱文件 (*.chess)"));
    if(fileName.isNull())
        return;
    emit open();
    std::fstream fin(fileName.toStdString());
    int row,col;
    while(!game->posStack.empty())
        game->posStack.pop();
    game->gameMapVec.clear();
    for (int i = 0; i < kBoardSizeNum; i++)
    {
        std::vector<int> lineBoard;
        for (int j = 0; j < kBoardSizeNum; j++)
            lineBoard.push_back(0);
        game->gameMapVec.push_back(lineBoard);
    }
    while(fin>>row>>col)
        game->posStack.push(std::make_pair(row,col));
    game->gameStatus = OPEN;
    fin.close();
}

void MainWindow::showNext()
{
    if(!game->posStack.empty())
    {
        game->gameMapVec[game->posStack.top().first][game->posStack.top().second] = game->playerFlag?1:-1;
        game->posStack.pop();
        game->playerFlag = !game->playerFlag;
        update();
        if(game->posStack.empty())
            emit finish();
    }
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    // 绘制棋盘
    painter.setRenderHint(QPainter::Antialiasing, true); // 抗锯齿

    for (int i = 0; i < kBoardSizeNum; i++)
    {
        painter.drawLine(kBoardMargin + kBlockSize * i, kBoardMargin, kBoardMargin + kBlockSize * i, kBoardMargin + kBlockSize * (kBoardSizeNum - 1));
        painter.drawLine(kBoardMargin, kBoardMargin + kBlockSize * i, kBoardMargin + kBlockSize * (kBoardSizeNum - 1), kBoardMargin + kBlockSize * i);
    }

    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    // 绘制落子标记(防止鼠标出框越界)
    if (clickPosRow >= 0 && clickPosRow < kBoardSizeNum &&
            clickPosCol >= 0 && clickPosCol < kBoardSizeNum &&
            game->gameMapVec[clickPosRow][clickPosCol] == 0)
    {
        if (game->playerFlag)
            brush.setColor(Qt::white);
        else
            brush.setColor(Qt::black);
        painter.setBrush(brush);
        painter.drawRect(kBoardMargin + kBlockSize * clickPosCol - kMarkSize / 2, kBoardMargin + kBlockSize * clickPosRow - kMarkSize / 2, kMarkSize, kMarkSize);
    }

    // 绘制棋子
    for (int i = 0; i < kBoardSizeNum; i++)
        for (int j = 0; j < kBoardSizeNum; j++)
        {
            if (game->gameMapVec[i][j] == 1)
            {
                brush.setColor(Qt::white);
                painter.setBrush(brush);
                painter.drawEllipse(kBoardMargin + kBlockSize * j - kRadius, kBoardMargin + kBlockSize * i - kRadius, kRadius * 2, kRadius * 2);
            }
            else if (game->gameMapVec[i][j] == -1)
            {
                brush.setColor(Qt::black);
                painter.setBrush(brush);
                painter.drawEllipse(kBoardMargin + kBlockSize * j - kRadius, kBoardMargin + kBlockSize * i - kRadius, kRadius * 2, kRadius * 2);
            }
        }

    // 判断输赢
    if (clickPosRow >= 0 && clickPosRow < kBoardSizeNum &&
            clickPosCol >= 0 && clickPosCol < kBoardSizeNum &&
            (game->gameMapVec[clickPosRow][clickPosCol] == 1 ||
             game->gameMapVec[clickPosRow][clickPosCol] == -1))
    {
        if (game->isWin(clickPosRow, clickPosCol) && game->gameStatus == PLAYING)
        {
            qDebug() << "win";
            game->gameStatus = WIN;
            QString str;
            if (game->gameMapVec[clickPosRow][clickPosCol] == 1)
                str = "白棋";
            else if (game->gameMapVec[clickPosRow][clickPosCol] == -1)
                str = "黑棋";
            if(!QMessageBox::information(this, "游戏结束", str + "获胜!是否要保存棋谱", tr("是"), tr("否")))
                saveGame();
            // 重置游戏状态，否则容易死循环
            game->startGame(game_type);
            game->gameStatus = PLAYING;
        }
    }


    // 判断死局
    if (game->isDeadGame())
    {
        QMessageBox::StandardButton btnValue = QMessageBox::information(this, "oops", "dead game!");
        if (btnValue == QMessageBox::Ok)
        {
            game->startGame(game_type);
            game->gameStatus = PLAYING;
        }
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(game->gameStatus == OPEN){
        clickPosRow = -1;
        clickPosCol = -1;
        return;
    }
    // 通过鼠标的hover确定落子的标记
    int x = event->x();
    int y = event->y();

    // 棋盘边缘不能落子
    if (x >= kBoardMargin - kBlockSize / 2 &&
            x < size().width() - kBoardMargin &&
            y >= kBoardMargin - kBlockSize / 2 &&
            y < size().height()- kBoardMargin)
    {
        // 获取最近的左上角的点
        int col = x / kBlockSize;
        int row = y / kBlockSize;

        int leftTopPosX = kBoardMargin + kBlockSize * col;
        int leftTopPosY = kBoardMargin + kBlockSize * row;

        // 根据距离算出合适的点击位置,一共四个点，根据半径距离选最近的
        clickPosRow = -1; // 初始化最终的值
        clickPosCol = -1;
        int len = 0; // 计算完后取整就可以了

        // 确定一个误差在范围内的点，且只可能确定一个出来
        len = sqrt((x - leftTopPosX) * (x - leftTopPosX) + (y - leftTopPosY) * (y - leftTopPosY));
        if (len < kPosDelta)
        {
            clickPosRow = row;
            clickPosCol = col;
        }
        len = sqrt((x - leftTopPosX - kBlockSize) * (x - leftTopPosX - kBlockSize) + (y - leftTopPosY) * (y - leftTopPosY));
        if (len < kPosDelta)
        {
            clickPosRow = row;
            clickPosCol = col + 1;
        }
        len = sqrt((x - leftTopPosX) * (x - leftTopPosX) + (y - leftTopPosY - kBlockSize) * (y - leftTopPosY - kBlockSize));
        if (len < kPosDelta)
        {
            clickPosRow = row + 1;
            clickPosCol = col;
        }
        len = sqrt((x - leftTopPosX - kBlockSize) * (x - leftTopPosX - kBlockSize) + (y - leftTopPosY - kBlockSize) * (y - leftTopPosY - kBlockSize));
        if (len < kPosDelta)
        {
            clickPosRow = row + 1;
            clickPosCol = col + 1;
        }
    }

    // 存了坐标后也要重绘
    update();
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    // 人下棋，并且不能抢机器的棋
    if (!(game_type == BOT && !game->playerFlag))
    {
        chessOneByPerson();
        // 如果是人机模式，需要调用AI下棋
        if (game->gameType == BOT && !game->playerFlag)
        {
            // 用定时器做一个延迟
            QTimer::singleShot(kAIDelay, this, SLOT(chessOneByAI()));
        }
    }
}

void MainWindow::chessOneByPerson()
{
    // 根据当前存储的坐标下子
    // 只有有效点击才下子，并且该处没有子
    if (clickPosRow != -1 && clickPosCol != -1 && game->gameMapVec[clickPosRow][clickPosCol] == 0)
    {
        game->actionByPerson(clickPosRow, clickPosCol);
        update();
    }
}

void MainWindow::chessOneByAI()
{
    game->actionByAI(clickPosRow, clickPosCol);
    update();
}
