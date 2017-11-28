// list_logs.cpp
//
// Select a Rivendell Log
//
//   (C) Copyright 2002-2004,2016 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public
//   License along with this program; if not, write to the Free Software
//   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <qpushbutton.h>

#include <rdadd_log.h>
#include <rddb.h>
#include <rdescape_string.h>

#include <globals.h>
#include <list_logs.h>

ListLogs::ListLogs(LogPlay *log,QWidget *parent)
  : QDialog(parent,"",true)
{
  //
  // Fix the Window Size
  //
  setMinimumSize(sizeHint());

  //
  // Generate Fonts
  //
  QFont button_font("Helvetica",12,QFont::Bold);
  button_font.setPixelSize(12);

  list_log=log;
  setCaption(tr("Select Log"));

  //
  // Filter Widget
  //
  list_filter_widget=
    new RDLogFilter(RDLogFilter::StationFilter,rduser,air_config,this);
  connect(list_filter_widget,SIGNAL(filterChanged(const QString &)),
	  this,SLOT(filterChangedData(const QString &)));

  //
  // Log List
  //
  list_log_list=new QListView(this);
  list_log_list->setAllColumnsShowFocus(true);
  list_log_list->setItemMargin(5);
  connect(list_log_list,
          SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),
          this,
          SLOT(doubleClickedData(QListViewItem *,const QPoint &,int)));
  list_log_list->addColumn(tr("NAME"));
  list_log_list->setColumnAlignment(0,Qt::AlignLeft);
  list_log_list->addColumn(tr("DESCRIPTION"));
  list_log_list->setColumnAlignment(1,Qt::AlignLeft);
  list_log_list->addColumn(tr("SERVICE"));
  list_log_list->setColumnAlignment(2,Qt::AlignLeft);

  //
  // Load Button
  //
  list_load_button=new QPushButton(tr("Load"),this);
  list_load_button->setFont(button_font);
  connect(list_load_button,SIGNAL(clicked()),this,SLOT(loadButtonData()));

  //
  // Unload Button
  //
  list_unload_button=new QPushButton(tr("Unload"),this);
  list_unload_button->setFont(button_font);
  connect(list_unload_button,SIGNAL(clicked()),this,SLOT(unloadButtonData()));

  //
  // Save Button
  //
  list_save_button=new QPushButton(tr("Save"),this);
  list_save_button->setFont(button_font);
  connect(list_save_button,SIGNAL(clicked()),this,SLOT(saveButtonData()));

  //
  // Save As Button
  //
  list_saveas_button=new QPushButton(tr("Save As"),this);
  list_saveas_button->setFont(button_font);
  connect(list_saveas_button,SIGNAL(clicked()),this,SLOT(saveAsButtonData()));

  //
  // Cancel Button
  //
  list_cancel_button=new QPushButton(tr("Cancel"),this);
  list_cancel_button->setFont(button_font);
  list_cancel_button->setDefault(true);
  connect(list_cancel_button,SIGNAL(clicked()),this,SLOT(cancelButtonData()));

  RefreshList();
}


QSize ListLogs::sizeHint() const
{
  return QSize(500,400);
}


QSizePolicy ListLogs::sizePolicy() const
{
  return QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
}


int ListLogs::exec(QString *logname,QString *svcname)
{
  list_logname=logname;
  list_svcname=svcname;
  list_saveas_button->setEnabled(rduser->createLog());
  QStringList services_list;
  QString sql=QString("select SERVICE_NAME from SERVICE_PERMS where ")+
    "STATION_NAME=\""+RDEscapeString(rdstation_conf->name())+"\"";
  RDSqlQuery *q=new RDSqlQuery(sql);
  services_list.push_back(tr("ALL"));
  while(q->next()) {
    services_list.push_back(q->value(0).toString());
  }
  delete q;
  RefreshList();

  return QDialog::exec();
}


void ListLogs::filterChangedData(const QString &where_sql)
{
  RefreshList();
}


void ListLogs::closeEvent(QCloseEvent *e)
{
  done(1);
}


void ListLogs::doubleClickedData(QListViewItem *,const QPoint &,int)
{
  loadButtonData();
}


void ListLogs::loadButtonData()
{
  QListViewItem *item=list_log_list->selectedItem();
  if(item==NULL) {
    return;
  }
  *list_logname=item->text(0);
  done(0);
}


void ListLogs::saveButtonData()
{
  if(list_logname->isEmpty()) {
    saveAsButtonData();
  }
  else {
    done(2);
  }
}


void ListLogs::saveAsButtonData()
{
  QString logname;
  QString svcname=*list_svcname;
  RDAddLog *log;
  log=new RDAddLog(&logname,&svcname,RDLogFilter::StationFilter,rduser,
		   rdstation_conf,tr("Rename Log"),this);

  if(log->exec()<0) {
    delete log;
    return;
  }
  delete log;
  *list_logname=logname;
  *list_svcname=svcname;
  done(3);
}


void ListLogs::unloadButtonData()
{
  done(-1);
}


void ListLogs::cancelButtonData()
{
  done(1);
}


void ListLogs::resizeEvent(QResizeEvent *e)
{
  list_filter_widget->setGeometry(10,10,size().width()-20,
				  list_filter_widget->sizeHint().height());
  list_log_list->setGeometry(10,list_filter_widget->sizeHint().height(),size().width()-20,size().height()-list_filter_widget->sizeHint().height()-80);
  list_load_button->setGeometry(10,size().height()-60,80,50);
  list_unload_button->setGeometry(100,size().height()-60,80,50);
  list_save_button->setGeometry(210,size().height()-60,80,50);
  list_saveas_button->setGeometry(300,size().height()-60,80,50);
  list_cancel_button->setGeometry(size().width()-90,size().height()-60,80,50);
}


void ListLogs::RefreshList()
{
  RDSqlQuery *q;
  QString sql;
  QListViewItem *l;
  QDate current_date=QDate::currentDate();
  QStringList services_list;

  list_log_list->clear();
  sql=QString("select NAME,DESCRIPTION,SERVICE from LOGS ")+
    "where (TYPE=0)&&(LOG_EXISTS=\"Y\")&&"+
    "((START_DATE<=\""+current_date.toString("yyyy-MM-dd")+"\")||"+
    "(START_DATE=\"0000-00-00\")||"+
    "(START_DATE is null))&&"+
    "((END_DATE>=\""+current_date.toString("yyyy-MM-dd")+"\")||"+
    "(END_DATE=\"0000-00-00\")||"+
    "(END_DATE is null))"+
    list_filter_widget->whereSql();
  q=new RDSqlQuery(sql);
  while(q->next()) {
    l=new QListViewItem(list_log_list);
    l->setText(0,q->value(0).toString());
    l->setText(1,q->value(1).toString());
    l->setText(2,q->value(2).toString());
  }
  delete q;
}
