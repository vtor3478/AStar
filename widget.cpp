#include "widget.h"
#include "ui_widget.h"

// 辅助地图，用于记录是否走过
int cellCostCnt = 0;
char walkMark[ROW][COL] = {0};
Cell_Struct *cellCost[ROW * COL] = {0};
QPoint walkDir[4] = {DIR_UP,DIR_DOWN,DIR_LEFT,DIR_RIGHT};
char snakeOccupy[ROW][COL] = {
    {1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1},
    {1,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,1},
    {1,0,0,0,0, 0,0,0,0,0, 1,0,0,0,0, 0,0,0,0,1},
    {1,0,0,0,0, 0,0,0,1,1, 1,0,0,0,0, 0,0,0,0,1},
    {1,0,0,0,0, 0,0,0,0,0, 1,0,0,0,0, 0,0,0,0,1},

    {1,0,0,0,0, 0,0,0,0,0, 1,0,0, 0,0,0,0,0,0,1},
    {1,0,0,0,0, 0,0,0,0,0, 1,1,1,1,1, 1,0,0,0,1},
    {1,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,1},
    {1,0,0,0,0, 0,0,0,0,0, 1,0,0,0,0, 0,0,0,0,1},
    {1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1},
};

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    startPos = QPoint(3,7);
    endPos = QPoint(5,13);

    prootCell = NULL;
    pnowCell = NULL;
    AStarInit();

    startBtn.setParent(this);
    startBtn.setText("点击重新开始寻路");
    startBtn.move(500,100);
    connect(&startBtn,&QPushButton::clicked,[=](){
        // 清空寻路情况，并重新开始寻路
        AStarInit();
    });
    runTimer.setInterval(10);
    //runTimer.start();
    connect(&runTimer,&QTimer::timeout,[=](){
        AStarSearch();
    });
    updateTimer.setInterval(50);
    updateTimer.start();
    connect(&updateTimer,&QTimer::timeout,[=](){
        update();
    });
}

void Widget::AStarInit()
{
    qDebug() << "现在清空所有数据，重新开始寻路。";
    // 先判断，判断是否存在历史残留的路径，需要释放内存
    if (NULL != pnowCell)
    {
        for (int dirIdex = 0; dirIdex < 4; ++dirIdex) {
            if (NULL != pnowCell->Childs[dirIdex]) {
                free(pnowCell->Childs[dirIdex]);
                //pnowCell->Childs[dirIdex] = NULL;
            }
        }
        // 变成父节点，并且逐层寻找，并
        pnowCell = pnowCell->parent;
    }
    pnowPos = route;
    for (int i = 0; i< ROUTE_LEN; ++i) {
        route[i] = QPoint(-1,-1);
    }
    memset(cellCost,0,sizeof(cellCost));
    cellCostCnt = 0;
    memset(walkMark,0,sizeof(walkMark));
    Cell_t* prootCell = (Cell_t*)malloc(sizeof(Cell_t));
    memset(prootCell,0,sizeof(Cell_t));
    prootCell->pos = startPos;
    prootCell->gCost = 0 * CELL_COST;
    prootCell->hCost = (endPos - prootCell->pos).manhattanLength() * CELL_COST;
    // 父节点，默认为起点
    pnowCell = prootCell;

    runTimer.start();
}
void Widget::AStarSave(void)
{
    Cell_t *pbackCell = pnowCell;
    while (pbackCell->pos != startPos) {
        *pnowPos = pbackCell->pos;
        ++pnowPos;
        pbackCell = pbackCell->parent;
    }
}

void Widget::PushCostCell(Cell_t* pCell)
{
    int emptyCellIdx = 0;
    // 找到空元素，直接载入
    for (int cellIdx = 0; cellIdx < ROW * COL; ++cellIdx) {
        if (NULL == cellCost[cellIdx]
                && 0 == emptyCellIdx) {
            emptyCellIdx = cellIdx;
            continue;
            //break;
        }
        if (NULL != cellCost[cellIdx]
                && pCell->pos == cellCost[cellIdx]->pos) {
            // 找到本身，直接返回
            emptyCellIdx = cellIdx;
            break;
        }
    }
    cellCost[emptyCellIdx] = pCell;
    qDebug() << "入buff[" << emptyCellIdx << "]："
             << pCell->pos << "，及其代价：" << pCell->gCost << "," << pCell->hCost ;
}

Cell_t* Widget::PopMinCostCell()
{
    // 从内存内寻找最小代价的cell
    Cell_t* pminCostCell = NULL;
    int minCellIdx = 0;
    // 下标为0的元素，可能是空的，所以需要查找到第一个非空元素
    while (NULL == cellCost[minCellIdx]
           && minCellIdx < ROW * COL) {
        ++minCellIdx;
    }
    for (int cellIdx = minCellIdx; cellIdx < ROW * COL; ++cellIdx)
    {
        if ( NULL == cellCost[cellIdx]) {
            continue;
        }
        if ( cellCost[minCellIdx]->gCost + cellCost[minCellIdx]->hCost
              > cellCost[cellIdx]->gCost + cellCost[cellIdx]->hCost)
        {
            minCellIdx = cellIdx;
        }

    }

    if (minCellIdx >= ROW * COL) {
        return NULL;
    }
    // 记录，并且从buffer内删除，然后返回最小的元素
    pminCostCell = cellCost[minCellIdx];
    qDebug() << "出buff[" << minCellIdx << "]：" << pminCostCell->pos
             << "，及其代价：" << pminCostCell->gCost << "," << pminCostCell->hCost ;
    cellCost[minCellIdx] = NULL;
    return  pminCostCell;
}

void Widget::AStarSearch(void)
{
    //while (0)
    //while (pnowCell->pos != endPos)
    {
        // 死胡同，回溯到了起点以前，说明并没有可以到达终点路径，直接返回
        if (NULL == pnowCell) {
            runTimer.stop();
            AStarInit();
            return;
        }
        // 解决起点与终点重合时，直接卡死的bug
        if (endPos == pnowCell->pos) {
            runTimer.stop();
            AStarSave();
            AStarInit();
            return;
        }
        for (int dirIdex = 0; dirIdex < 4; ++dirIdex) {
            int childX = (pnowCell->pos + walkDir[dirIdex]).x();
            int childY = (pnowCell->pos + walkDir[dirIdex]).y();

            // 如果即将访问的节点超出范围 或 已探索 或 是墙壁，那么跳过
            if (childX < 0 || ROW <= childX || childY < 0 || COL <= childY
                    || WALKED == walkMark[childX][childY]
                    || snakeOccupy[childX][childY]) {
                continue ;
            }
            // 先申请节点，，，，等找到最终路径，再一次性全部释放
            Cell_t* pchildCell = (Cell_t*)malloc(sizeof(Cell_t));
            memset(pchildCell,0,sizeof(Cell_t));
            // 建立指向父节点，建立父节点指向本节点，进行记录，便于以后释放
            pchildCell->parent = pnowCell;
            pnowCell->Childs[dirIdex] = pchildCell;
            // 计算坐标，计算代价，单位距离的代价是CELL_COST
            pchildCell->pos = pnowCell->pos + walkDir[dirIdex];
            pchildCell->gCost = pnowCell->gCost + CELL_COST;
            pchildCell->hCost = (endPos - pchildCell->pos).manhattanLength() * CELL_COST;
            // 将节点装入buffer
            PushCostCell(pchildCell);
        }
        // 寻找最小代价节点进行弹出
        pnowCell = PopMinCostCell();
        if (NULL == pnowCell) {
            runTimer.stop();
            return;
        }
        // 将路径记录下来
        walkMark[pnowCell->pos.x()][pnowCell->pos.y()] = WALKED;
        qDebug() << "所选基点：" << pnowCell->pos
                 << "，及其代价：" << pnowCell->gCost << "," << pnowCell->hCost ;
        // 使用当前最小代价节点，当作当前节点，，再次计算,
        //pnowCell = pnowCell->Childs[minCostDirIdx];
    }
}

Widget::~Widget()
{
    delete ui;
}

QPoint Change2Paint(QPoint srcPoint)
{
    QPoint dstPos;
    dstPos.setX(srcPoint.y());
    dstPos.setY(srcPoint.x());
    dstPos = dstPos * SCALE;
    return  dstPos;
}

void Widget::mousePressEvent(QMouseEvent *e)
{
    // 需要将鼠标点击坐标转换为二维数组坐标
    mousePos = e->pos();
    // qpoint类的除号重载，使用了浮点运算，且四舍五入
    mousePos.rx() /= SCALE;
    mousePos.ry() /= SCALE;
    // 在坐标系中的xy与数组中xy是相反的，切记。
    snakeOccupy[mousePos.y()][mousePos.x()]
            = ! snakeOccupy[mousePos.y()][mousePos.x()];
}

void Widget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(qRgb(255,255,255));
    // 黑色为墙，白色为空地
    for (int row = 0; row < ROW; row++) {
        for (int col = 0; col < COL; col++) {
            QPoint pos(row,col);
            snakeOccupy[row][col]
                    ? painter.setBrush(QBrush(qRgb(0,0,0)))
                    : painter.setBrush(QBrush(qRgb(255,255,255)));
            pos = Change2Paint(pos);
            painter.drawRect(QRect(pos,pos + QPoint(20,20)));
        }
    }

    // 蓝色为路径
    QPoint pos;
    QPoint* proute = route;
    painter.setBrush(QBrush(qRgb(0,255,0)));
    while (QPoint(-1,-1) != *proute) {
        pos = Change2Paint(*proute);
        painter.drawRect(QRect(pos,pos + QPoint(20,20)));
        ++proute;
    }
    // 红色为起点，绿色为终点
    painter.setBrush(QBrush(qRgb(255,0,0)));
    pos = Change2Paint(startPos);
    painter.drawRect(QRect(pos,pos + QPoint(20,20)));
    painter.setBrush(QBrush(qRgb(0,0,255)));
    pos = Change2Paint(endPos);
    painter.drawRect(QRect(pos,pos + QPoint(20,20)));

    // 左上黄色小格，代表探索过
    painter.setBrush(QBrush(qRgb(255,255,0)));
    for (int row = 0; row < ROW; row++) {
        for (int col = 0; col < COL; col++) {
            QPoint pos(row,col);
            if (walkMark[row][col]) {
                pos = Change2Paint(pos);
                painter.drawRect(QRect(pos,pos + QPoint(10,10)));
            }
        }
    }

}

