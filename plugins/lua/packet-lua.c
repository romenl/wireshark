/*
 * packet-lua.c
 *
 * Ethereal's interface to the Lua Programming Language
 *
 * (c) 2006, Luis E. Garcia Ontanon <luis.ontanon@gmail.com>
 *
 * $Id$
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "packet-lua.h"
static lua_State* L = NULL;


/* ethereal uses lua */

extern int lua_tap_packet(void *tapdata, packet_info *pinfo, epan_dissect_t *edt, const void *data _U_) {
    Tap tap = tapdata;
    
    lua_pushstring(L, "_ethereal_pinfo");
    pushPinfo(L, pinfo);
    lua_settable(L, LUA_GLOBALSINDEX);
    
    lua_tree = edt->tree;
    
    lua_dostring(L,ep_strdup_printf("taps.%s(_ethereal_pinfo);",tap->name));
    
    return 1;
}

void lua_tap_reset(void *tapdata) {
    Tap tap = tapdata;
    lua_dostring(L,ep_strdup_printf("tap_resets.%s();",tap->name));
}

void lua_tap_draw(void *tapdata) {
    Tap tap = tapdata;
    lua_dostring(L,ep_strdup_printf("tap_draws.%s();",tap->name));
}


void dissect_lua(tvbuff_t* tvb, packet_info* pinfo, proto_tree* tree) {
    lua_pushstring(L, "_ethereal_tvb");
    pushTvb(L, tvb);
    lua_settable(L, LUA_GLOBALSINDEX);

    lua_pushstring(L, "_ethereal_pinfo");
    pushPinfo(L, pinfo);
    lua_settable(L, LUA_GLOBALSINDEX);

    lua_pushstring(L, "_ethereal_tree");
    pushTree(L, tree);
    lua_settable(L, LUA_GLOBALSINDEX);
    
    lua_pinfo = pinfo;
    
    lua_dostring(L,ep_strdup_printf("dissectors.%s(_ethereal_tvb,_ethereal_pinfo,_ethereal_tree);",pinfo->current_proto));
    
    lua_pinfo = NULL;
}

static void init_lua(void) {
    if (L)
        lua_dostring(L, "for k in init_routines do init_routines[k]() end;");
}

void proto_reg_handoff_lua(void) {
    lua_data_handle = find_dissector("data");
    
    if (L)
        lua_dostring(L, "for k in handoff_routines do handoff_routines[k]() end ;");
}

static const char *getF(lua_State *L _U_, void *ud, size_t *size)
{
    FILE *f=(FILE *)ud;
    static char buff[512];
    if (feof(f)) return NULL;
    *size=fread(buff,1,sizeof(buff),f);
    return (*size>0) ? buff : NULL;
}

extern void lua_functions_defined_but_unused(void) {
    toValueString(L,1);
    toProtoField(L,1);
    toProtoFieldArray(L,1);
    toEtt(L,1);
    toEttArray(L,1);
    toProto(L,1);
    toByteArray(L,1);
    toTvb(L,1);
    toColumn(L,1);
    toPinfo(L,1);
    toTree(L,1);
    toItem(L,1);
    toDissector(L,1);
    toDissectorTable(L,1);
    toInteresting(L,1);
    toTap(L,1);
    toColumns(L,1);
}

void proto_register_lua(void)
{
    FILE* file;
    gchar* filename = getenv("ETHEREAL_LUA_INIT");
    
    /* TODO: 
        disable if not running with the right credentials
        
        if (euid == 0 && euid != ruid) return;
    */
    
    if (!filename) filename = get_persconffile_path("init.lua", FALSE);

    file = fopen(filename,"r");

    if (! file) return;
    
    register_init_routine(init_lua);    
    
    L = lua_open();
    luaopen_base(L);
    luaopen_table(L);
    luaopen_io(L);
    luaopen_string(L);
    ValueString_register(L);
    ProtoField_register(L);
    ProtoFieldArray_register(L);
    Ett_register(L);
    EttArray_register(L);
    ByteArray_register(L);
    Tvb_register(L);
    Proto_register(L);
    Column_register(L);
    Pinfo_register(L);
    Tree_register(L);
    Item_register(L);
    Dissector_register(L);
    DissectorTable_register(L);
    Interesting_register(L);
    Columns_register(L);
    Tap_register(L);
    lua_pushstring(L, "handoff_routines");
    lua_newtable (L);
    lua_settable(L, LUA_GLOBALSINDEX);
    
    lua_pushstring(L, "init_routines");
    lua_newtable (L);
    lua_settable(L, LUA_GLOBALSINDEX);
    
    lua_pushstring(L, "dissectors");
    lua_newtable (L);
    lua_settable(L, LUA_GLOBALSINDEX);
    
    lua_pushstring(L, "taps");
    lua_newtable (L);
    lua_settable(L, LUA_GLOBALSINDEX);

    lua_pushstring(L, "tap_resets");
    lua_newtable (L);
    lua_settable(L, LUA_GLOBALSINDEX);

    lua_pushstring(L, "tap_draws");
    lua_newtable (L);
    lua_settable(L, LUA_GLOBALSINDEX);
    
    if (lua_load(L,getF,file,filename) || lua_pcall(L,0,0,0))
        fprintf(stderr,"%s\n",lua_tostring(L,-1));
    
    fclose(file);
    
    lua_data_handle = NULL;
    lua_pinfo = NULL;
    lua_tree = NULL;
    
}

