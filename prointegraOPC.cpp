/*
 *  Prointegra OPC
 *
 *  Copyright 2016 by it's authors. 
 *
 *  Some rights reserved. See COPYING, AUTHORS.
 *  This file may be used under the terms of the GNU General Public
 *  License version 3.0 as published by the Free Software Foundation
 *  and appearing in the file COPYING included in the packaging of
 *  this file.
 *
 *  This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 *  WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/**
@file capture.c
*/

#include "prointegraOPC.h"


/*! Constructor*/
ProintegraOPC::ProintegraOPC()
{
  int i = 0;
  //load xml config file
  confParser = new ConfigParser("config/database.xml","config/slaves.xml");
  //retrieve database info from config file
  confParser->retrieveDBParams();
  //creating database interfaces
  nDBs = confParser->retnDBs();
  hDatabase = new DBInterface*[nDBs];
  for(i=0;i<nDBs;i++)
    {
      hDatabase[i] = new DBInterface();
      hDatabase[i]->setup(confParser->retDBParams(i),confParser->retDBTables(i));
    }
  //retrieve slaves info from config file
  confParser->retrieveCommParams();
  nSlaves = confParser->retnSlaves();
  hSlaves = new CommInterface*[nSlaves];
  for(i=0;i<nSlaves;i++)
    {
      hSlaves[i] = new CommInterface();
      hSlaves[i]->setup(confParser->retSlaveParams(i));
    }  

  return;   
}
/*! destructor*/
/*!TODO: it's in fact not be called, when we close with crtl+c the application doesn't take the sigterm signal*/
ProintegraOPC::~ProintegraOPC()
{
  delete confParser;
  for(int i=nDBs-1;i== 0 ; i--)
    delete hDatabase[nDBs];
  delete hDatabase;
  return;
}

/*checking database devices and creating it's tables if don't exists*/
int ProintegraOPC::checkDB()
{
  cout<<"DEBUG: checking and creating!" << endl;
  //check database
  //  for(int i = 0;i < nDBs ;i++)
  //  hDatabase[i]->checkAndCreate();
  //check comunications
  return 0;   
}

/*!checking communication processes if running or not
TODO: it's in fact still not be used*/
int ProintegraOPC::checkComm()
{
  for(int i=0; i < nSlaves; i++)
    {
      std::cout << "DEBUG:checking slave "<< i << " status: " << commDaemonManager->checkDaemon(i)<< std::endl;
    }

  return 0;   
}
/*data capturing process
it should takes data from communications mailboxes and save it to it's slave structure*/
int ProintegraOPC::dataCapture()
{
  for(int i=0; i < nSlaves; i++)
    hSlaves[i]->readData();
  return 0;   
}
/*data to database process
it takes data from slave structures and save it to our tables structures
TODO: it has to be cut in smaller functions!*/
int ProintegraOPC::dataToDB()
{
  int slave = 0;
  int field = 0;
  //databases
  for(int i=0; i< nDBs ; i++)
    {
      //tables
      for(int j=0; j < hDatabase[i]->retNumTables();j++)
	{
	  //fields
	  for(int k=0; k < hDatabase[i]->retNumFields(j);k++)
	    {
	      //std::cout <<"DEBUG: number of fields ofr database:"<< i <<",table:"<<j<<" = "<< hDatabase[i]->retNumFields(j) << std::endl;
	      //std::cout <<"DEBUG: iterating: database=" << i << " table=" << j <<" field=" << k << " slave=" << slave << " tag=" << field << std::endl;
	      std::cout << hDatabase[i]->retFieldTag(j,k) << " , " << hSlaves[slave]->retTagName(field) << std::endl; 
	      if(slave >= nSlaves)
		{
		  slave = 0;
		  field = 0;
		  hDatabase[i]->setFieldValid(j,k,0);
		  k++;
		}
	      //not equal
	      while(strcmp(hDatabase[i]->retFieldTag(j,k),hSlaves[slave]->retTagName(field)))
		{
		  field++;
		  if(field >= hSlaves[slave]->retNumTags())
		    {
		      slave++;
		      field = 0;
		    }
		  if(slave >= nSlaves)
		    {
		      slave = 0;
		      field = 0;
		      hDatabase[i]->setFieldValid(j,k,0);
		      break;
		    }		  
		}
	      if(!strcmp(hDatabase[i]->retFieldTag(j,k),hSlaves[slave]->retTagName(field)))
		{
		  if(hSlaves[slave]->retTagValid(field))
		    {
		      hDatabase[i]->setFieldValid(j,k,1);
		      hDatabase[i]->setFieldValue(j,k,hSlaves[slave]->retTagValue(field));
		    }
		  else
		    hDatabase[i]->setFieldValid(j,k,0);	    
		}
	      field++;
			   
	      if(field >= hSlaves[slave]->retNumTags())
		{
		  slave++;
		}
	     
	    }
	}
    }

  return 0;   
}
/*start communications
*/
int ProintegraOPC::startCommunications()
{
  commDaemonManager = new CommDaemon(nSlaves);

  for(int i=0; i < nSlaves; i++)
    commDaemonManager->launchDaemon(i,confParser->retSlaveParams(i).commId,confParser->retSlaveParams(i).commType);
  return 0;   
}

/*capturing proccess
TODO: we don't manage daemons!*/
int ProintegraOPC::startCapture()
{


  std::cout << "capturing!" << std::endl;
  while(1)
    {
      Sleep(2000);
      checkComm();
      dataCapture();
      dataToDB();
      showDBData();
    }
  return 0;   
}

//DEBUG FUNCTIONS
/*show data in memory structures, data to be written in databases*/
int ProintegraOPC::showDBData()
{
  int slave = 0;
  int field = 0;
  //databases
  for(int i=0; i< nDBs ; i++)
    {
      std::cout << "**************************************************" << std::endl;
      std::cout << "DEBUG: showing data in memory, to save in databases" << std::endl;
      std::cout << std::endl;
      std::cout << "DATA IN DATABASE: " << i+1 << std::endl;
      //tables
      for(int j=0; j < hDatabase[i]->retNumTables();j++)
	{
	  std::cout << std::endl;
	  std::cout << "--->TABLE: " << j+1 << std::endl;
	  //fields
	  for(int k=0; k < hDatabase[i]->retNumFields(j);k++)
	    {
	      std::cout << "------->FIELD: " << k+1 << std::endl;
	      std::cout << "--------->NAME: " << hDatabase[i]->retFieldTag(j,k)  << std::endl;
	      std::cout << "--------->VALUE: " << hDatabase[i]->retFieldValue(j,k)  << std::endl;
	      std::cout << "--------->VALID: " << hDatabase[i]->retFieldValid(j,k)  << std::endl;	      
	    }
	}
    }

}


