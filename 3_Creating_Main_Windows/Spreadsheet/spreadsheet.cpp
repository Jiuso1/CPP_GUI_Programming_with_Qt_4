#include <QtGui>

#include "cell.h"
#include "spreadsheet.h"
#include <QMessageBox>
#include <QApplication>
#include <algorithm>

Spreadsheet::Spreadsheet(QWidget *parent) : QTableWidget(parent){
    autoRecalc = true;

    setItemPrototype(new Cell);
    setSelectionMode(ContiguousSelection);

    connect(this,SIGNAL(itemChanged(QTableWidgetItem*)),
            this,SLOT(somethingChanged()));

    clear();
}

void Spreadsheet::clear(){
    setRowCount(0);
    setColumnCount(0);
    setRowCount(RowCount);
    setColumnCount(ColumnCount);

    for(int i = 0;i < ColumnCount;i++){
        QTableWidgetItem *item = new QTableWidgetItem;
        item->setText(QString(QChar('A'+i)));
        setHorizontalHeaderItem(i,item);
    }

    setCurrentCell(0,0);
}

Cell* Spreadsheet::cell(int row,int column) const{
    return static_cast<Cell*>(item(row,column));
}

QString Spreadsheet::text(int row,int column) const{
    Cell *c = cell(row,column);
    if(c){
        return c->text();//Si c no apunta a nullptr, retorna el texto de c.
    }else{
        return "";
    }
}

QString Spreadsheet::formula(int row,int column) const{
    Cell *c = cell(row,column);
    if(c){
        return c->formula();
    }else{
        return "";
    }
}

void Spreadsheet::setFormula(int row,int column,const QString &formula){
    Cell *c = cell(row,column);
    if(!c){
        c = new Cell;
        setItem(row,column,c);
    }
    c->setFormula(formula);
}

QString Spreadsheet::currentLocation() const{
    return QChar('A' + currentColumn()) + QString::number(currentRow()+1);
}

QString Spreadsheet::currentFormula() const{
    return formula(currentRow(),currentColumn());
}

void Spreadsheet::somethingChanged(){
    if(autoRecalc){
        recalculate();
    }
    emit modified();
}

bool Spreadsheet::writeFile(const QString &fileName){
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly)){
        QMessageBox::warning(this,tr("Spreadsheet"),tr("Cannot write file %1:\n%2.").arg(file.fileName()).arg(file.errorString()));
        //Cuando te falte una librería, consulta qué debes incluir en la ayuda de Qt. Recuerda APSO.
        return false;
    }
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_4);

    out<<quint32(MagicNumber);

    QApplication::setOverrideCursor(Qt::WaitCursor);

    for(int row = 0; row < RowCount;row++){
        for(int column = 0; column < ColumnCount;column++){
            QString str = formula(row,column);
            if(!str.isEmpty()){
                out<<quint16(row)<<quint16(column)<<str;
            }
        }
    }
    QApplication::restoreOverrideCursor();
    return true;
}

bool Spreadsheet::readFile(const QString &fileName){
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly)){
        QMessageBox::warning(this,tr("Spreadsheet"),tr("Cannot read file %1:\n%2.").arg(file.fileName()).arg(file.errorString()));
        return false;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_4);

    quint32 magic;
    in >> magic;
    if(magic != MagicNumber){
        QMessageBox::warning(this,tr("Spreadsheet"),tr("The file is not a Spreadsheet file."));
        return false;
    }

    clear();

    quint16 row;
    quint16 column;
    QString str;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    while(!in.atEnd()){
        in>>row>>column>>str;
        setFormula(row,column,str);
    }
    QApplication::restoreOverrideCursor();
    return true;
}

void Spreadsheet::cut(){
    copy();
    del();
}

void Spreadsheet::copy(){
    QTableWidgetSelectionRange range = selectedRange();
    QString str;

    for(int i = 0;i < range.rowCount();i++){
        if(i > 0){
            str += "\n";
        }
        for(int j = 0;j < range.columnCount();j++){
            if(j > 0){
                str += "\t";
            }
            str += formula(range.topRow() + i,range.leftColumn() + j);
        }
    }

    QApplication::clipboard()->setText(str);
}

QTableWidgetSelectionRange Spreadsheet::selectedRange() const{
    QList<QTableWidgetSelectionRange> ranges = selectedRanges();
    if(ranges.isEmpty()){
        return QTableWidgetSelectionRange();
    }
    return ranges.first();
}

void Spreadsheet::paste(){
    QTableWidgetSelectionRange range = selectedRange();
    QString str = QApplication::clipboard()->text();
    QStringList rows = str.split('\n');
    int numRows = rows.count();
    int numColumns = rows.first().count('\t') + 1;

    if(range.rowCount() * range.columnCount() != -1
        && (range.rowCount() != numRows
            || range.columnCount() != numColumns)){
        QMessageBox::information(this,tr("Spreadsheet"),tr("The information cannot be pasted because the copy and paste areas aren't the same size."));
        return;//Curioso el return en métodos void. Sirve para acabar antes, pero como es void no podemos retornar nada.
    }

    for(int i = 0;i < numRows;i++){
        QStringList columns = rows[i].split('\t');
        for(int j = 0;j < numColumns;j++){
            int row = range.topRow() + i;
            int column = range.leftColumn() + j;
            if(row < RowCount && column < ColumnCount){
                setFormula(row,column,columns[j]);
            }
        }
    }

    somethingChanged();
}

void Spreadsheet::del(){
    QList<QTableWidgetItem *> items = selectedItems();
    if(!items.isEmpty()){
        foreach(QTableWidgetItem *item,items){
            delete item;
        }
        somethingChanged();
    }
}

void Spreadsheet::selectCurrentRow(){
    selectRow(currentRow());
}

void Spreadsheet::selectCurrentColumn(){
    selectColumn(currentColumn());
}

void Spreadsheet::findNext(const QString &str,Qt::CaseSensitivity cs){
    int row = currentRow();
    int column = currentColumn() + 1;

    while(row < RowCount){
        while(column < ColumnCount){
            if(text(row,column).contains(str,cs)){
                clearSelection();
                setCurrentCell(row,column);
                activateWindow();
                return;
            }
            column++;
        }
        column = 0;
        row++;
    }

    QApplication::beep();
}

void Spreadsheet::findPrevious(const QString &str,Qt::CaseSensitivity cs){
    int row = currentRow();
    int column = currentColumn() - 1;

    while(row >= 0){
        while(column >= 0){
            if(text(row,column).contains(str,cs)){
                clearSelection();
                setCurrentCell(row,column);
                activateWindow();
                return;
            }
            column--;
        }
        column = ColumnCount - 1;
        row--;
    }

    QApplication::beep();
}

void Spreadsheet::recalculate(){
    for(int row = 0;row < RowCount;row++){
        for(int column = 0;column < ColumnCount;column++){
            if(cell(row,column)){
                cell(row,column)->setDirty();
            }
            viewport()->update();
        }
    }
}

void Spreadsheet::setAutoRecalculate(bool recalc){
    autoRecalc = recalc;
    qDebug()<<"autoRecal vale: "<<autoRecalc;
    if(autoRecalc){
        recalculate();
    }
}

void Spreadsheet::sort(const SpreadsheetCompare &compare){
    QList<QStringList> rows;
    QTableWidgetSelectionRange range = selectedRange();
    int i;

    for(i = 0;i < range.rowCount();i++){
        QStringList row;
        for(int j = 0;j < range.columnCount();j++){
            row.append(formula(range.topRow() + i,range.leftColumn() + j));
        }
        rows.append(row);
    }

    //qStableSort(rows.begin(), rows.end(), compare);//Imposible ejecutarlo en Qt 6 ya.
    std::stable_sort(rows.begin(), rows.end(), compare);

    for(i = 0; i < range.rowCount();i++){
        for(int j = 0;j < range.columnCount();j++){
            setFormula(range.topRow() + i, range.leftColumn() + j, rows[i][j]);
        }
    }

    clearSelection();
    somethingChanged();
}

bool SpreadsheetCompare::operator()(const QStringList &row1,const QStringList &row2) const{
    for(int i = 0;i < keyCount;i++){
        int column = keys[i];
        if(column != -1){
            if(row1[column] != row2[column]){
                if(ascending[i]){
                    return row1[column] < row2[column];
                }else{
                    return row1[column] > row2[column];
                }
            }
        }
    }
    return false;
}
