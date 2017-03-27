/*  =========================================================================
    fty_mdns_sd_server - 42ity mdns sd server

    Copyright (C) 2014 - 2017 Eaton                                        
                                                                           
    This program is free software; you can redistribute it and/or modify   
    it under the terms of the GNU General Public License as published by   
    the Free Software Foundation; either version 2 of the License, or      
    (at your option) any later version.                                    
                                                                           
    This program is distributed in the hope that it will be useful,        
    but WITHOUT ANY WARRANTY; without even the implied warranty of         
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
    GNU General Public License for more details.                           
                                                                           
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.            
    =========================================================================
*/

/*
@header
    fty_mdns_sd_server - 42ity mdns sd server
@discuss
@end
*/

#include "fty_mdns_sd_classes.h"
#include "avahi_wrapper.h" 

#define TIMEOUT_MS 1000   //wait at least 30 seconds

//  Structure of our class

    
struct _fty_mdns_sd_server_t {
    bool verbose;            // is actor verbose or not
    char *name;              // actor name
    mlm_client_t *client;    // malamute client
    AvahiWrapper *service; // service mDNS-SD
    //default service announcement definition
    char *srv_name;
    char *srv_type;
    char *srv_stype;
    char *srv_port;
    char *fty_info_command;
    zhash_t *map_txt;
};

//free dynamic item
static void s_destroy_txt(void *arg){
    char *value=(char*)arg;
    zstr_free(&value);
}

static void
s_set_txt_record(fty_mdns_sd_server_t *self,const char *key,const char *value)
{
    if(value==NULL) return;
    zsys_debug ("s_set_txt_record(%s,%s)",key,value);
    char *_value = strdup(value);
    int rv=zhash_insert(self->map_txt,key,_value);
    if(rv==-1)
        zhash_update(self->map_txt,key,_value);
    zhash_freefn(self->map_txt,key,s_destroy_txt);
}

static void
s_set_srv_name(fty_mdns_sd_server_t *self,const char *value)
{
    if(value==NULL) return;
    //delete previous dyn value
    zstr_free (&self->srv_name);
    char *_value = strdup(value);
    self->srv_name = _value;

}

static void
s_set_srv_port(fty_mdns_sd_server_t *self,const char *value)
{
    if(value==NULL) return;
    //delete previous dyn value
    zstr_free (&self->srv_port);
    char *_value = strdup(value);
    self->srv_port = _value;

}

static void
s_set_srv_type(fty_mdns_sd_server_t *self,const char *value)
{
    if(value==NULL) return;
    //delete previous dyn value
    zstr_free (&self->srv_type);
    char *_value = strdup(value);
    self->srv_type = _value;

}

static void
s_set_srv_stype(fty_mdns_sd_server_t *self,const char *value)
{
    if(value==NULL) return;
    //delete previous dyn value
    zstr_free (&self->srv_stype);
    char *_value = strdup(value);
    self->srv_stype = _value;

}
//  --------------------------------------------------------------------------
//  Create a new fty_mdns_sd_server

fty_mdns_sd_server_t *
fty_mdns_sd_server_new (const char* name)
{
    fty_mdns_sd_server_t *self = (fty_mdns_sd_server_t *) zmalloc (sizeof (fty_mdns_sd_server_t));
    assert (self);
    //  Initialize class properties here
    self->verbose = false;
    if (!name) {
        zsys_error ("Address for fty_mdns_sd actor is NULL");
        free (self);
        return NULL;
    }
    self->name = strdup (name);
    self->client = mlm_client_new();
    self->service = new AvahiWrapper(); // service mDNS-SD
    self->map_txt = zhash_new();
    //do minimal initialization
    s_set_txt_record(self,"uuid",
        "00000000-0000-0000-0000-000000000000");

    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the fty_mdns_sd_server

void
fty_mdns_sd_server_destroy (fty_mdns_sd_server_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        fty_mdns_sd_server_t *self = *self_p;
        //  Free class properties here
        zstr_free (&self->name);
        zstr_free (&self->srv_name);
        zstr_free (&self->srv_type);
        zstr_free (&self->srv_stype);
        zstr_free (&self->srv_port);
        zstr_free (&self->fty_info_command);
        mlm_client_destroy (&self->client);
        zhash_destroy (&self->map_txt); 
        delete self->service;
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}




static int
s_poll_fty_info(fty_mdns_sd_server_t *self)
{
    assert (self);
    
    zmsg_t *send = zmsg_new ();
    zmsg_addstr (send, self->fty_info_command);
    zuuid_t *uuid = zuuid_new ();
    zmsg_addstr (send, zuuid_str_canonical (uuid));
    zsys_debug ("requesting fty-info ..");
    if(mlm_client_sendto(self->client,"fty-info","info", NULL, 1000, &send)!=0)
    {
        zsys_error("info: client->sendto (address = '%s') failed.", "fty-info");
        zmsg_destroy(&send);
        zuuid_destroy(&uuid);
        return -2;
    }
        
    zmsg_t *resp = mlm_client_recv(self->client);    
    if (!resp)
    {
        zsys_error ("info: client->recv (timeout = '5') returned NULL");
        return -3;
    }
    
    char *zuuid_reply = zmsg_popstr (resp);
    //TODO : check UUID if you think it is important
    zstr_free(&zuuid_reply);
    zuuid_destroy(&uuid);
    
    zframe_t *frame_infos = zmsg_next (resp);
    zhash_t *infos = zhash_unpack(frame_infos);
    
    s_set_txt_record(self, "uuid", (char*)zhash_lookup (infos, "uuid"));
    s_set_txt_record(self, "name", (char*)zhash_lookup (infos, "name"));
    s_set_txt_record(self, "model",(char*)zhash_lookup (infos, "model"));
    s_set_txt_record(self, "vendor", (char*)zhash_lookup (infos, "vendor"));
    s_set_txt_record(self, "serial", (char*)zhash_lookup (infos, "serial"));
    s_set_txt_record(self, "location", (char*)zhash_lookup (infos, "location"));
    s_set_txt_record(self, "path", (char*)zhash_lookup (infos, "rest_path"));
    
    s_set_srv_port(self, (char *) zhash_lookup (infos, "rest_port"));

    zhash_destroy(&infos);
    
    zmsg_destroy(&resp);
    zmsg_destroy(&send);
    
    return 0;
}

//  --------------------------------------------------------------------------
//  Create a new fty_mdns_sd_server

void
fty_mdns_sd_server (zsock_t *pipe, void *args)
{
    char*name=(char*) args;
    fty_mdns_sd_server_t *self = fty_mdns_sd_server_new(name);
    assert (self);

    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe (self->client), NULL);
    assert (poller);
    
    // do not forget to send a signal to actor :)
    zsock_signal (pipe, 0); 
    
    zsys_info ("fty-mdns-sd-server: Started",self->name);
    // wait for interval left
    void *which = zpoller_wait (poller, -1);
    while (!zsys_interrupted) {
            
        if (which == pipe) {
            zmsg_t *message = zmsg_recv (pipe);
            if (!message)
                break;
            char *command = zmsg_popstr (message);
            if (!command) {
                zsys_warning ("Empty command.");
                continue;
            }
            if (self->verbose)
                zsys_debug ("actor command=%s", command);
            if (streq(command, "$TERM")) {
                zsys_info ("Got $TERM");
                zstr_free (&command);
                zmsg_destroy (&message);
                break;
            }
            else if (streq (command, "VERBOSE")) {
                self->verbose = true;
            }
            else if (streq (command, "CONNECT")) {
                char *endpoint = zmsg_popstr (message);
                 if (!endpoint)
                    zsys_error ("%s:\tMissing endpoint", self->name);
                assert (endpoint);
                int r = mlm_client_connect (self->client, endpoint, 5000, self->name);
                if (r == -1)
                    zsys_error ("%s:\tConnection to endpoint '%s' failed", self->name, endpoint);
                zsys_debug("fty-mdns-sd-server: CONNECT %s/%s",endpoint,self->name);
                zstr_free (&endpoint);
            }
            else if (streq (command, "SET-DEFAULT-SERVICE")) {
                 //set new ones
                char *name  = zmsg_popstr (message);
                char *type  = zmsg_popstr (message);
                char *stype = zmsg_popstr (message);
                char *port  = zmsg_popstr (message);
                zsys_debug("fty-mdns-sd-server: SET-DEFAULT-SERVICE [%s,%s,%s,%s]",
                    name,type,stype,port) ;
                s_set_srv_name(self,name);
                s_set_srv_type(self,type);
                s_set_srv_stype(self,stype);
                s_set_srv_port(self,port);
                zstr_free(&name);
                zstr_free(&type);
                zstr_free(&stype);
                zstr_free(&port);
                
            }
            else if (streq (command, "SET-DEFAULT-TXT")) {
                char *key   = zmsg_popstr (message);
                char *value = zmsg_popstr (message);
                zsys_debug("fty-mdns-sd-server: SET-DEFAULT-TXT %s=%s",
                    key,value) ;
                 s_set_txt_record(self,key,value);
                zstr_free(&key);
                zstr_free(&value);
                
            }
            else if (streq (command, "DO-DEFAULT-ANNOUNCE")) {
                //free previous value
                zstr_free (&self->fty_info_command);
                //get info from fty-info
                self->fty_info_command = zmsg_popstr (message);
                zsys_debug("fty-mdns-sd-server: DO-DEFAULT-ANNOUNCE %s",
                        self->fty_info_command);
                s_poll_fty_info(self);
                self->service->setServiceDefinition(
                    self->srv_name,
                    self->srv_type,
                    self->srv_stype,
                    self->srv_port,
                    (char*)zhash_lookup (self->map_txt, "uuid"));
                //set all txt properties
                char *value = (char *) zhash_first (self->map_txt);   // first value
                while ( value != NULL )
                {
                    char *key = (char *) zhash_cursor (self->map_txt);   // key of this value
                    self->service->setTxtRecords (key,value);
                    value = (char *) zhash_next (self->map_txt);   // next value
                }
                self->service->start();
            }
            
            else
                zsys_warning ("%s:\tUnkown API command=%s, ignoring", self->name, command);
            zstr_free (&command);
            zmsg_destroy (&message);
        }// end of comand pipe processing

    }

    self->service->stop();
    fty_mdns_sd_server_destroy (&self);
    zpoller_destroy (&poller);
    
}
//  --------------------------------------------------------------------------
//  Self test of this class

void
fty_mdns_sd_server_test (bool verbose)
{
    printf (" * fty_mdns_sd_server: ");

    //  @selftest
    //  Simple create/destroy test

    zactor_t *server = zactor_new (fty_mdns_sd_server, (void*)"fty-mdns-sd-test");
    assert (server);
    
    zstr_sendx (server, "CONNECT", "ipc://@/malamute", NULL);
    
    zclock_sleep (1000);
    
    
    zstr_sendx (server, "SET-DEFAULT-SERVICE",
            "IPC","_https._tcp.","_powerservice._sub._https._tcp.","443", NULL);
    zstr_sendx (server, "SET-DEFAULT-TXT",
            "txtvers","1.0.0",NULL);
    
    //do first announcement
    //this test needs avahi-deamon running
    //zstr_sendx (server, "DO-DEFAULT-ANNOUNCE", "INFO",NULL);

    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}