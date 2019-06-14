#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include "gamemodel.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);

signals:
    void finish();
    void open();

protected:
    //绘制
    void paintEvent(QPaintEvent *event);
    // 监听鼠标移动情况，方便落子
    void mouseMoveEvent(QMouseEvent *event);
    // 实际落子
    void mouseReleaseEvent(QMouseEvent *event);

private:
    GameModel *game; // 游戏指针
    GameType game_type; // 存储游戏类型
    int clickPosRow, clickPosCol; // 存储将点击的位置
    void initGame();
    void checkGame(int y, int x);
    void saveGame();

private slots:
    void chessOneByPerson(); // 人执行
    void chessOneByAI(); // AI下棋

    void initPVPGame();
    void initPVEGame();

    void openGame();
    void quitGame();
    void showIntro();

    void regret();
    void showNext();
};

#endif // MAINWINDOW_H
