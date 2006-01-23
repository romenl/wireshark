#include "packet-lua.h"

LUA_CLASS_DEFINE(Proto,PROTO,if (! *p) luaL_error(L,"null Proto"));
LUA_CLASS_DEFINE(ProtoField,PROTO_FIELD,if (! *p) luaL_error(L,"null ProtoField"));
LUA_CLASS_DEFINE(ProtoFieldArray,PROTO_FIELD_ARRAY,if (! *p) luaL_error(L,"null ProtoFieldArray"));
LUA_CLASS_DEFINE(Ett,ETT,NOP);
LUA_CLASS_DEFINE(EttArray,ETT_ARRAY,if (! *p) luaL_error(L,"null EttArray"));
LUA_CLASS_DEFINE(ValueString,VALUE_STRING,NOP);

/*
 * ProtoField class
 */

static const eth_ft_types_t ftenums[] = {
{"FT_BOOLEAN",FT_BOOLEAN},
{"FT_UINT8",FT_UINT8},
{"FT_UINT16",FT_UINT16},
{"FT_UINT24",FT_UINT24},
{"FT_UINT32",FT_UINT32},
{"FT_UINT64",FT_UINT64},
{"FT_INT8",FT_INT8},
{"FT_INT16",FT_INT16},
{"FT_INT24",FT_INT24},
{"FT_INT32",FT_INT32},
{"FT_INT64",FT_INT64},
{"FT_FLOAT",FT_FLOAT},
{"FT_DOUBLE",FT_DOUBLE},
{"FT_STRING",FT_STRING},
{"FT_STRINGZ",FT_STRINGZ},
{"FT_ETHER",FT_ETHER},
{"FT_BYTES",FT_BYTES},
{"FT_UINT_BYTES",FT_UINT_BYTES},
{"FT_IPv4",FT_IPv4},
{"FT_IPv6",FT_IPv6},
{"FT_IPXNET",FT_IPXNET},
{"FT_FRAMENUM",FT_FRAMENUM},
{"FT_GUID",FT_GUID},
{"FT_OID",FT_OID},
{NULL,FT_NONE}
};

static enum ftenum get_ftenum(const gchar* type) {
    const eth_ft_types_t* ts;
    for (ts = ftenums; ts->str; ts++) {
        if ( g_str_equal(ts->str,type) ) {
            return ts->id;
        }
    }
    
    return FT_NONE;
}

static const gchar* ftenum_to_string(enum ftenum ft) {
    const eth_ft_types_t* ts;
    for (ts = ftenums; ts->str; ts++) {
        if ( ts->id == ft ) {
            return ts->str;
        }
    }
    
    return NULL;
}

struct base_display_string_t {
    const gchar* str;
    base_display_e base;
};

static const struct base_display_string_t base_displays[] = {
	{ "BASE_NONE", BASE_NONE},
	{"BASE_DEC", BASE_DEC},
	{"BASE_HEX", BASE_HEX},
	{"BASE_OCT", BASE_OCT},
	{"BASE_DEC_HEX", BASE_DEC_HEX},
	{"BASE_HEX_DEC", BASE_HEX_DEC},
	{NULL,0}
};

static const gchar* base_to_string(base_display_e base) {
    const struct base_display_string_t* b;
    for (b=base_displays;b->str;b++) {
        if ( base == b->base)
            return b->str;
    }
    return NULL;
}

static base_display_e string_to_base(const gchar* str) {
    const struct base_display_string_t* b;
    for (b=base_displays;b->str;b++) {
        if ( g_str_equal(str,b->str))
            return b->base;
    }
    return BASE_NONE;
}



static int ProtoField_new(lua_State* L) {
    ProtoField f = g_malloc(sizeof(eth_field_t));
    GArray* vs;
    
    f->hfid = -2;
    f->name = g_strdup(luaL_checkstring(L,1));
    f->abbr = g_strdup(luaL_checkstring(L,2));
    f->type = get_ftenum(luaL_checkstring(L,3));
    
    if (f->type == FT_NONE) {
        luaL_argerror(L, 3, "invalid FT_type");
        return 0;
    }
    
    if (! lua_isnil(L,4) ) {
        vs = checkValueString(L,4);
        
        if (vs) {
            f->vs = (value_string*)vs->data;
        }
    } else {
        f->vs = NULL;
    }
    
    /* XXX: need BASE_ERROR */
    f->base = string_to_base(luaL_optstring(L, 5, "BASE_NONE"));
    f->mask = luaL_optint(L, 6, 0x0);
    f->blob = g_strdup(luaL_optstring(L,7,""));
    
    pushProtoField(L,f);
    
    return 1;
}




static int ProtoField_integer(lua_State* L, enum ftenum type) {
    ProtoField f = g_malloc(sizeof(eth_field_t));
    const gchar* abbr = luaL_checkstring(L,1); 
    const gchar* name = luaL_optstring(L,2,abbr);
    const gchar* base = luaL_optstring(L, 3, "BASE_DEC");
    GArray* vs = (lua_gettop(L) >= 4) ? checkValueString(L,4) : NULL;
    int mask = luaL_optint(L, 5, 0x0);
    const gchar* blob = luaL_optstring(L,6,"");
    
    f->hfid = -2;
    f->name = g_strdup(name);
    f->abbr = g_strdup(abbr);
    f->type = type;
    f->vs = (value_string*)vs->data;
    f->base = string_to_base(base);
    f->mask = mask;
    f->blob = g_strdup(blob);
    
    pushProtoField(L,f);
    
    return 1;
}

#define PROTOFIELD_INTEGER(lower,FT) static int ProtoField_##lower(lua_State* L) { return ProtoField_integer(L,FT); }
PROTOFIELD_INTEGER(uint8,FT_UINT8);
PROTOFIELD_INTEGER(uint16,FT_UINT16);
PROTOFIELD_INTEGER(uint24,FT_UINT24);
PROTOFIELD_INTEGER(uint32,FT_UINT32);
PROTOFIELD_INTEGER(uint64,FT_UINT64);
PROTOFIELD_INTEGER(int8,FT_INT8);
PROTOFIELD_INTEGER(int16,FT_INT8);
PROTOFIELD_INTEGER(int24,FT_INT8);
PROTOFIELD_INTEGER(int32,FT_INT8);
PROTOFIELD_INTEGER(int64,FT_INT8);
PROTOFIELD_INTEGER(framenum,FT_FRAMENUM);

static int ProtoField_other(lua_State* L,enum ftenum type) {
    ProtoField f = g_malloc(sizeof(eth_field_t));
    const gchar* abbr = luaL_checkstring(L,1); 
    const gchar* name = luaL_optstring(L,2,abbr);
    const gchar* blob = luaL_optstring(L,3,"");
    
    f->hfid = -2;
    f->name = g_strdup(name);
    f->abbr = g_strdup(abbr);
    f->type = type;
    f->vs = NULL;
    f->base = ( type == FT_FLOAT || type == FT_DOUBLE) ? BASE_DEC : BASE_NONE;
    f->mask = 0;
    f->blob = g_strdup(blob);
    
    pushProtoField(L,f);
    
    return 1;
}

#define PROTOFIELD_OTHER(lower,FT) static int ProtoField_##lower(lua_State* L) { return ProtoField_other(L,FT); }
PROTOFIELD_OTHER(ipv4,FT_IPv4);
PROTOFIELD_OTHER(ipv6,FT_IPv6);
PROTOFIELD_OTHER(ipx,FT_IPXNET);
PROTOFIELD_OTHER(ether,FT_ETHER);
PROTOFIELD_OTHER(bool,FT_BOOLEAN);
PROTOFIELD_OTHER(float,FT_FLOAT);
PROTOFIELD_OTHER(double,FT_DOUBLE);
PROTOFIELD_OTHER(string,FT_STRING);
PROTOFIELD_OTHER(stringz,FT_STRINGZ);
PROTOFIELD_OTHER(bytes,FT_BYTES);
PROTOFIELD_OTHER(ubytes,FT_UINT_BYTES);
PROTOFIELD_OTHER(guid,FT_GUID);
PROTOFIELD_OTHER(oid,FT_OID);


static int ProtoField_tostring(lua_State* L) {
    ProtoField f = checkProtoField(L,1);
    gchar* s = g_strdup_printf("ProtoField(%i): %s %s %s %s %p %.8x %s",f->hfid,f->name,f->abbr,ftenum_to_string(f->type),base_to_string(f->base),f->vs,f->mask,f->blob);
    
    lua_pushstring(L,s);
    g_free(s);
    
    return 1;
}


static const luaL_reg ProtoField_methods[] = {
    {"new",   ProtoField_new},
    {"uint8",ProtoField_uint8},
    {"uint16",ProtoField_uint16},
    {"uint24",ProtoField_uint24},
    {"uint32",ProtoField_uint32},
    {"uint64",ProtoField_uint64},
    {"int8",ProtoField_int8},
    {"int16",ProtoField_int16},
    {"int24",ProtoField_int24},
    {"int32",ProtoField_int32},
    {"int64",ProtoField_int64},
    {"framenum",ProtoField_framenum},
    {"ipv4",ProtoField_ipv4},
    {"ipv6",ProtoField_ipv6},
    {"ipx",ProtoField_ipx},
    {"ether",ProtoField_ether},
    {"bool",ProtoField_bool},
    {"float",ProtoField_float},
    {"double",ProtoField_double},
    {"string",ProtoField_string},
    {"stringz",ProtoField_stringz},
    {"bytes",ProtoField_bytes},
    {"ubytes",ProtoField_ubytes},
    {"guid",ProtoField_guid},
    {"oid",ProtoField_oid},
    {0,0}
};

static const luaL_reg ProtoField_meta[] = {
    {"__tostring", ProtoField_tostring},
    {0, 0}
};

int ProtoField_register(lua_State* L) {
    const eth_ft_types_t* ts;
    const struct base_display_string_t* b;
    
    luaL_openlib(L, PROTO_FIELD, ProtoField_methods, 0);
    luaL_newmetatable(L, PROTO_FIELD);
    luaL_openlib(L, 0, ProtoField_meta, 0);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pushliteral(L, "__metatable");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pop(L, 1);
    
    /* add a global FT_* variable for each FT_ type */
    for (ts = ftenums; ts->str; ts++) {
        lua_pushstring(L, ts->str);
        lua_pushstring(L, ts->str);
        lua_settable(L, LUA_GLOBALSINDEX);
    }
    
    /* add a global BASE_* variable for each BASE_ */
    for (b=base_displays;b->str;b++) {
        lua_pushstring(L, b->str);
        lua_pushstring(L, b->str);
        lua_settable(L, LUA_GLOBALSINDEX);
    }
    
    return 1;
}


/*
 * ProtoFieldArray class
 */


static int ProtoFieldArray_new(lua_State* L) {
    ProtoFieldArray fa;
    guint i;
    guint num_args = lua_gettop(L);
    
    fa = g_array_new(TRUE,TRUE,sizeof(hf_register_info));
    
    for ( i = 1; i <= num_args; i++) {
        ProtoField f = checkProtoField(L,i);
        hf_register_info hfri = { &(f->hfid), {f->name,f->abbr,f->type,f->base,VALS(f->vs),f->mask,f->blob,HFILL}};
        
        if (f->hfid != -2) {
            luaL_argerror(L, i, "field has already been added to an array");
            return 0;
        }
        
        f->hfid = -1;
        
        g_array_append_val(fa,hfri);
    }
    
    pushProtoFieldArray(L,fa);
    return 1;
}


static int ProtoFieldArray_add(lua_State* L) {
    ProtoFieldArray fa = checkProtoFieldArray(L,1);
    guint i;
    guint num_args = lua_gettop(L);
    
    for ( i = 2; i <= num_args; i++) {
        ProtoField f = checkProtoField(L,i);
        hf_register_info hfri = { &(f->hfid), {f->name,f->abbr,f->type,f->base,VALS(f->vs),f->mask,f->blob,HFILL}};
        
        if (f->hfid != -2) {
            luaL_argerror(L, i, "field has already been added to an array");
            return 0;
        }
        
        f->hfid = -1;
        
        g_array_append_val(fa,hfri);
    }
    
    return 0;
}

static int ProtoFieldArray_tostring(lua_State* L) {
    GString* s = g_string_new("ProtoFieldArray:\n");
    hf_register_info* f;
    ProtoFieldArray fa = checkProtoFieldArray(L,1);
    unsigned i;
    
    for(i = 0; i< fa->len; i++) {
        f = &(((hf_register_info*)(fa->data))[i]);
        g_string_sprintfa(s,"%i %s %s %s %u %p %.8x %s\n",*(f->p_id),f->hfinfo.name,f->hfinfo.abbrev,ftenum_to_string(f->hfinfo.type),f->hfinfo.display,f->hfinfo.strings,f->hfinfo.bitmask,f->hfinfo.blurb);
    };
    
    lua_pushstring(L,s->str);
    g_string_free(s,TRUE);
    
    return 1;
}

static int ProtoFieldArray_gc(lua_State* L) {
    ProtoFieldArray vs = checkValueString(L,1);
    
    g_array_free(vs,TRUE);
    
    return 0;
}


static const luaL_reg ProtoFieldArray_methods[] = {
    {"new",   ProtoFieldArray_new},
    {"add",   ProtoFieldArray_add},
    {0,0}
};

static const luaL_reg ProtoFieldArray_meta[] = {
    {"__gc",       ProtoFieldArray_gc},
    {"__tostring", ProtoFieldArray_tostring},
    {0, 0}
};

int ProtoFieldArray_register(lua_State* L) {
    luaL_openlib(L, PROTO_FIELD_ARRAY, ProtoFieldArray_methods, 0);
    luaL_newmetatable(L, PROTO_FIELD_ARRAY);
    luaL_openlib(L, 0, ProtoFieldArray_meta, 0);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pushliteral(L, "__metatable");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pop(L, 1);
    
    return 1;
}




/*
 * Ett class
 */


static int Ett_new(lua_State* L) {
    Ett e = g_malloc(sizeof(int));
    *e = -2;
    pushEtt(L,e);
    
    return 1;
}

static int Ett_tostring(lua_State* L) {
    Ett e = checkEtt(L,1);
    gchar* s = g_strdup_printf("Ett: %i",*e);
    
    lua_pushstring(L,s);
    g_free(s);
    
    return 1;
}


static const luaL_reg Ett_methods[] = {
    {"new",   Ett_new},
    {0,0}
};

static const luaL_reg Ett_meta[] = {
    {"__tostring", Ett_tostring},
    {0, 0}
};

int Ett_register(lua_State* L) {
    luaL_openlib(L, ETT, Ett_methods, 0);
    luaL_newmetatable(L, ETT);
    luaL_openlib(L, 0, Ett_meta, 0);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pushliteral(L, "__metatable");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pop(L, 1);
    
    return 1;
}





/*
 * EttArray class
 */

static int EttArray_new(lua_State* L) {
    EttArray ea = g_array_new(TRUE,TRUE,sizeof(gint*));
    guint i;
    guint num_args = lua_gettop(L);
    
    for (i = 1; i <= num_args; i++) {
        Ett e = checkEtt(L,i);
        
        if(*e != -2) {
            luaL_argerror(L, i, "SubTree has already been added to an array");
            return 0;
        }
        
        *e = -1;
        
        g_array_append_val(ea,e);
    }
    
    pushEttArray(L,ea);
    return 1;
}


static int EttArray_add(lua_State* L) {
    EttArray ea = checkEttArray(L,1);
    guint i;
    guint num_args = lua_gettop(L);
    
    for (i = 2; i <= num_args; i++) {
        Ett e = checkEtt(L,i);
        if(*e != -2) {
            luaL_argerror(L, i, "SubTree has already been added to an array");
            return 0;
        }
        
        *e = -1;
        
        g_array_append_val(ea,e);
    }
    
    return 0;
}

static int EttArray_tostring(lua_State* L) {
    GString* s = g_string_new("EttArray:\n");
    EttArray ea = checkEttArray(L,1);
    unsigned i;
    
    for(i = 0; i< ea->len; i++) {
        gint ett = *(((gint**)(ea->data))[i]);
        g_string_sprintfa(s,"%i\n",ett);
    };
    
    lua_pushstring(L,s->str);
    g_string_free(s,TRUE);
    
    return 1;
}

static int EttArray_register_to_ethereal(lua_State* L) {
    EttArray ea = checkEttArray(L,1);
    
    if (!ea->len) {
        luaL_argerror(L,1,"empty array");
        return 0;
    }
    
    /* is last ett -1? */
    if ( *(((gint *const *)ea->data)[ea->len -1])  != -1) {
        luaL_argerror(L,1,"array has been registered already");
        return 0;
    }
    
    proto_register_subtree_array((gint *const *)ea->data, ea->len);
    return 0;
}

static int EttArray_gc(lua_State* L) {
    EttArray ea = checkEttArray(L,1);
    
    g_array_free(ea,FALSE);
    
    return 0;
}


static const luaL_reg EttArray_methods[] = {
    {"new",   EttArray_new},
    {"add",   EttArray_add},
    {"register",   EttArray_register_to_ethereal},
    {0,0}
};

static const luaL_reg EttArray_meta[] = {
    {"__gc",       EttArray_gc},
    {"__tostring", EttArray_tostring},
    {0, 0}
};

int EttArray_register(lua_State* L) {
    luaL_openlib(L, ETT_ARRAY, EttArray_methods, 0);
    luaL_newmetatable(L, ETT_ARRAY);
    luaL_openlib(L, 0, EttArray_meta, 0);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pushliteral(L, "__metatable");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pop(L, 1);
    
    return 1;
}




/*
 * Proto class
 */


static int Proto_new(lua_State* L) {
    Proto proto;
    const gchar* name = luaL_checkstring(L,1);
    const gchar* filter = luaL_checkstring(L,2);
    const gchar* desc = luaL_checkstring(L,3);
    
    if (! (name && filter && desc) ) return 0;
    
    proto = g_malloc(sizeof(eth_proto_t));
    
    proto->hfid = -1;
    proto->name = g_strdup(name);
    proto->filter = g_strdup(filter);
    proto->desc = g_strdup(desc);
    proto->hfarray = NULL;
    proto->prefs_module = NULL;
    proto->prefs = NULL;
    proto->handle = NULL;
    proto->is_postdissector = FALSE;
    
    if (proto->name && proto->filter && proto->desc) {
        if ( proto_get_id_by_filter_name(proto->filter) > 0 ) { 
            g_free(proto);
            luaL_argerror(L,2,"Protocol exists already");
            return 0;
        } else {
            proto->hfid = proto_register_protocol(proto->desc,proto->name,proto->filter);
            proto->handle = create_dissector_handle(dissect_lua,proto->hfid);
            pushProto(L,proto);
            return 1;
        }
     } else {
         if (! proto->name ) 
             luaL_argerror(L,1,"missing name");
         
         if (! proto->filter ) 
             luaL_argerror(L,2,"missing filter");
         
         if (! proto->desc ) 
             luaL_argerror(L,3,"missing desc");
         
         g_free(proto);

         return 0;
     }

}

static int Proto_register_field_array(lua_State* L) {
    Proto proto = checkProto(L,1);
    ProtoFieldArray fa = checkProtoFieldArray(L,2);
    
    if (!proto) {
        luaL_argerror(L,1,"not a good proto");
        return 0;
    }

    if (! fa) {
        luaL_argerror(L,2,"not a good field_array");
        return 0;
    }

    if( ! fa->len ) {
        luaL_argerror(L,2,"empty field_array");
        return 0;
    }

    if (proto->hfarray) {
        luaL_argerror(L,1,"field_array already registered for this protocol");
    }
        
    proto->hfarray = (hf_register_info*)(fa->data);
    proto_register_field_array(proto->hfid,proto->hfarray,fa->len);
    
    return 0;
}

static int Proto_add_uint_pref(lua_State* L) {
    Proto proto = checkProto(L,1);
    gchar* abbr = g_strdup(luaL_checkstring(L,2));
    guint def = (guint)luaL_optint(L,3,0);
    guint base = (guint)luaL_optint(L,4,10);
    gchar* name = g_strdup(luaL_optstring (L, 5, ""));
    gchar* desc = g_strdup(luaL_optstring (L, 6, ""));
    
    eth_pref_t* pref = g_malloc(sizeof(eth_pref_t));
    pref->name = abbr;
    pref->type = PREF_UINT;
    pref->value.u = def;
    pref->next = NULL;
    
    if (! proto->prefs_module)
        proto->prefs_module = prefs_register_protocol(proto->hfid, NULL);
    
    if (! proto->prefs) {
        proto->prefs = pref;
    } else {
        eth_pref_t* p;
        for (p = proto->prefs; p->next; p = p->next) ;
        p->next = pref;
    }
    
    prefs_register_uint_preference(proto->prefs_module, abbr,name,
                                   desc, base, &(pref->value.u));
    
    return 0;
}

static int Proto_add_bool_pref(lua_State* L) {
    Proto proto = checkProto(L,1);
    gchar* abbr = g_strdup(luaL_checkstring(L,2));
    gboolean def = (gboolean)luaL_optint(L,3,FALSE);
    gchar* name = g_strdup(luaL_optstring (L, 4, ""));
    gchar* desc = g_strdup(luaL_optstring (L, 5, ""));
    
    eth_pref_t* pref = g_malloc(sizeof(eth_pref_t));
    pref->name = abbr;
    pref->type = PREF_BOOL;
    pref->value.b = def;
    pref->next = NULL;
    
    if (! proto->prefs_module)
        proto->prefs_module = prefs_register_protocol(proto->hfid, NULL);
    
    if (! proto->prefs) {
        proto->prefs = pref;
    } else {
        eth_pref_t* p;
        for (p = proto->prefs; p->next; p = p->next) ;
        p->next = pref;
    }
    
    prefs_register_bool_preference(proto->prefs_module, abbr,name,
                                   desc, &(pref->value.b));
        
    return 0;
}

static int Proto_add_string_pref(lua_State* L) {
    Proto proto = checkProto(L,1);
    gchar* abbr = g_strdup(luaL_checkstring(L,2));
    gchar* def = g_strdup(luaL_optstring (L, 3, ""));
    gchar* name = g_strdup(luaL_optstring (L, 4, ""));
    gchar* desc = g_strdup(luaL_optstring (L, 5, ""));
    
    eth_pref_t* pref = g_malloc(sizeof(eth_pref_t));
    pref->name = abbr;
    pref->type = PREF_STRING;
    pref->value.s = def;
    pref->next = NULL;
    
    if (! proto->prefs_module)
        proto->prefs_module = prefs_register_protocol(proto->hfid, NULL);
    
    if (! proto->prefs) {
        proto->prefs = pref;
    } else {
        eth_pref_t* p;
        for (p = proto->prefs; p->next; p = p->next) ;
        p->next = pref;
    }
    
    prefs_register_string_preference(proto->prefs_module, abbr,name,
                                     desc, &(pref->value.s));
        
    
    return 0;
}


static int Proto_get_pref(lua_State* L) {
    Proto proto = checkProto(L,1);
    const gchar* abbr = luaL_checkstring(L,2);

    if (!proto) {
        luaL_argerror(L,1,"not a good proto");
        return 0;
    }
    
    if (!abbr) {
        luaL_argerror(L,2,"not a good abbrev");
        return 0;
    }
    
    if (proto->prefs) {
        eth_pref_t* p;
        for (p = proto->prefs; p; p = p->next) {
            if (g_str_equal(p->name,abbr)) {
                switch(p->type) {
                    case PREF_BOOL:
                        lua_pushboolean(L, p->value.b);
                        break;
                    case PREF_UINT:
                        lua_pushnumber(L, (lua_Number)(p->value.u));
                        break;
                    case PREF_STRING:
                        lua_pushstring(L, p->value.s);
                        break;
                }
                return 1;
            }
        }
        
        luaL_argerror(L,2,"no such preference for this protocol");
        return 0;
        
    } else {
        luaL_error(L,"no preferences set for this protocol");
        return 0;
    }
}


static int Proto_tostring(lua_State* L) { 
    Proto proto = checkProto(L,1);
    gchar* s;
    
    if (!proto) return 0;
    
    s = g_strdup_printf("Proto: %s",proto->name);
    lua_pushstring(L,s);
    g_free(s);
    
    return 1;
}

static int Proto_register_postdissector(lua_State* L) { 
    Proto proto = checkProto(L,1);
    if (!proto) return 0;
    
    if(!proto->is_postdissector) {
        register_postdissector(proto->handle);
    } else {
        luaL_argerror(L,1,"this protocol is already registered as postdissector");
    }
    
    return 0;
}


static int Proto_get_dissector(lua_State* L) { 
    Proto proto = checkProto(L,1);
    if (!proto) return 0;
    
    if (proto->handle) {
        pushDissector(L,proto->handle);
        return 1;
    } else {
        luaL_error(L,"The protocol hasn't been registered yet");
        return 0;
    }
}

static const luaL_reg Proto_methods[] = {
    {"new",   Proto_new},
    {"register_field_array",   Proto_register_field_array},
    {"add_uint_pref",   Proto_add_uint_pref},
    {"add_bool_pref",   Proto_add_bool_pref},
    {"add_string_pref",   Proto_add_string_pref},
    {"get_pref",   Proto_get_pref},
    {"register_as_postdissector",   Proto_register_postdissector},
    {"get_dissector",   Proto_get_dissector},
    {0,0}
};

static const luaL_reg Proto_meta[] = {
    {"__tostring", Proto_tostring},
    {0, 0}
};

int Proto_register(lua_State* L) {
    luaL_openlib(L, PROTO, Proto_methods, 0);
    luaL_newmetatable(L, PROTO);
    luaL_openlib(L, 0, Proto_meta, 0);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pushliteral(L, "__metatable");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pop(L, 1);
    
    return 1;
}

LUA_CLASS_DEFINE(Dissector,DISSECTOR,NOP);
LUA_CLASS_DEFINE(DissectorTable,DISSECTOR_TABLE,NOP);


/*
 * Dissector class
 */

static int Dissector_get (lua_State *L) {
    const gchar* name = luaL_checkstring(L,1);
    Dissector d;
    
    if (!name) {
        return 0;
    }
    
    if ((d = find_dissector(name))) {
        pushDissector(L, d);
        return 1;
    } else {
        luaL_argerror(L,1,"No such dissector");
        return 0;
    }
    
}

static int Dissector_call(lua_State* L) {
    Dissector d = checkDissector(L,1);
    Tvb tvb = checkTvb(L,2);
    Pinfo pinfo = checkPinfo(L,3);
    Tree tree = checkTree(L,4);
    
    if (! ( d && tvb && pinfo) ) return 0;
    
    call_dissector(d, tvb, pinfo, tree);
    return 0;
}


static int Dissector_tostring(lua_State* L) {
    Dissector d = checkDissector(L,1);
    if (!d) return 0;
    lua_pushstring(L,dissector_handle_get_short_name(d));
    return 1;
}


static const luaL_reg Dissector_methods[] = {
    {"get", Dissector_get },
    {"call", Dissector_call },
    {0,0}
};

static const luaL_reg Dissector_meta[] = {
    {"__tostring", Dissector_tostring},
    {0, 0}
};

int Dissector_register(lua_State* L) {
    luaL_openlib(L, DISSECTOR, Dissector_methods, 0);
    luaL_newmetatable(L, DISSECTOR);
    luaL_openlib(L, 0, Dissector_meta, 0);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pushliteral(L, "__metatable");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pop(L, 1);
    
    return 1;
};






/*
 * DissectorTable class
 */
static int DissectorTable_new (lua_State *L) {
    gchar* name = (void*)luaL_checkstring(L,1);
    gchar* ui_name = (void*)luaL_optstring(L,2,name);
    const gchar* ftstr = luaL_optstring(L,3,"FT_UINT32");
    enum ftenum type;
    base_display_e base = luaL_optint(L,4,BASE_DEC);
    
    if(!(name && ui_name && ftstr)) return 0;
    
    name = g_strdup(name);
    ui_name = g_strdup(ui_name);
    type = get_ftenum(ftstr);
    
    switch(type) {
        case FT_STRING:
            base = BASE_NONE;
        case FT_UINT32:
        {
            DissectorTable dt = g_malloc(sizeof(struct _eth_distbl_t));
            
            dt->table = register_dissector_table(name, ui_name, type, base);
            dt->name = name;
            pushDissectorTable(L, dt);
        }
            return 1;
        default:
            luaL_argerror(L,3,"Invalid ft_type");
            return 0;
    }
    
}

static int DissectorTable_get (lua_State *L) {
    const gchar* name = luaL_checkstring(L,1);
    dissector_table_t table;
    
    if(!name) return 0;
    
    table = find_dissector_table(name);
    
    if (table) {
        DissectorTable dt = g_malloc(sizeof(struct _eth_distbl_t));
        dt->table = table;
        dt->name = g_strdup(name);
        
        pushDissectorTable(L, dt);
        
        return 1;
    } else {
        luaL_error(L,"No such dissector_table");
        return 0;
    }
    
}


static int DissectorTable_add (lua_State *L) {
    DissectorTable dt = checkDissectorTable(L,1);
    Proto p = checkProto(L,3);
    ftenum_t type;
    
    if (!(dt && p)) return 0;
    
    type = get_dissector_table_selector_type(dt->name);
    
    if (type == FT_STRING) {
        gchar* pattern = g_strdup(luaL_checkstring(L,2));
        dissector_add_string(dt->name, pattern,p->handle);
    } else if ( type == FT_UINT32 ) {
        int port = luaL_checkint(L, 2);
        dissector_add(dt->name, port, p->handle);
    }
    
    return 0;
}


static int DissectorTable_try (lua_State *L) {
    DissectorTable dt = checkDissectorTable(L,1);
    Tvb tvb = checkTvb(L,3);
    Pinfo pinfo = checkPinfo(L,4);
    Tree tree = checkTree(L,5);
    ftenum_t type;
    
    if (! (dt && tvb && pinfo && tree) ) return 0;
    
    type = get_dissector_table_selector_type(dt->name);
    
    if (type == FT_STRING) {
        const gchar* pattern = luaL_checkstring(L,2);
        
        if (!pattern) return 0;
        
        if (dissector_try_string(dt->table,pattern,tvb,pinfo,tree))
            return 0;
    } else if ( type == FT_UINT32 ) {
        int port = luaL_checkint(L, 2);
        if (dissector_try_port(dt->table,port,tvb,pinfo,tree))
            return 0;
    } else {
        luaL_error(L,"No such type of dissector_table");
    }
    
    call_dissector(lua_data_handle,tvb,pinfo,tree);
    return 0;
}

static int DissectorTable_get_dissector (lua_State *L) {
    DissectorTable dt = checkDissectorTable(L,1);
    ftenum_t type;
    dissector_handle_t handle = lua_data_handle;
    
    if (!dt) return 0;
    
    type = get_dissector_table_selector_type(dt->name);
    
    if (type == FT_STRING) {
        const gchar* pattern = luaL_checkstring(L,2);
        handle = dissector_get_string_handle(dt->table,pattern);
    } else if ( type == FT_UINT32 ) {
        int port = luaL_checkint(L, 2);
        handle = dissector_get_port_handle(dt->table,port);
    }
    
    pushDissector(L,handle);
    
    return 1;
    
}


static int DissectorTable_tostring(lua_State* L) {
    DissectorTable dt = checkDissectorTable(L,1);
    GString* s;
    ftenum_t type;
    
    if (!dt) return 0;
    
    type =  get_dissector_table_selector_type(dt->name);
    s = g_string_new("DissectorTable ");
    
    switch(type) {
        case FT_STRING:
        {
            g_string_sprintfa(s,"%s String:\n",dt->name);
            break;
        }
        case FT_UINT32:
        {
            int base = get_dissector_table_base(dt->name);
            g_string_sprintfa(s,"%s Integer(%i):\n",dt->name,base);
            break;
        }
        default:
            luaL_error(L,"Strange table type");
    }            
    
    lua_pushstring(L,s->str);
    g_string_free(s,TRUE);
    return 1;
}

static const luaL_reg DissectorTable_methods[] = {
    {"new", DissectorTable_new},
    {"get", DissectorTable_get},
    {"add", DissectorTable_add },
    {"try", DissectorTable_try },
    {"get_dissector", DissectorTable_get_dissector },
    {0,0}
};

static const luaL_reg DissectorTable_meta[] = {
    {"__tostring", DissectorTable_tostring},
    {0, 0}
};

int DissectorTable_register(lua_State* L) {
    luaL_openlib(L, DISSECTOR_TABLE, DissectorTable_methods, 0);
    luaL_newmetatable(L, DISSECTOR_TABLE);
    luaL_openlib(L, 0, DissectorTable_meta, 0);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pushliteral(L, "__metatable");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pop(L, 1);
    
    return 1;
};


/*
 * ValueString class
 */

static int ValueString_new(lua_State* L) {
    ValueString vs = g_array_new(TRUE,TRUE,sizeof(value_string));
    pushValueString(L,vs);
    return 1;
}


static int ValueString_add(lua_State* L) {
    ValueString vs = checkValueString(L,1);
    value_string v = {0,NULL};
    
    v.value = luaL_checkint(L,2);
    v.strptr = g_strdup(luaL_checkstring(L,3));
    
    g_array_append_val(vs,v);
    
    return 0;
}

static int ValueString_match(lua_State* L) {
    ValueString vs = checkValueString(L,1);
    guint32 val = (guint32)luaL_checkint(L,2);
    const gchar* def = luaL_optstring(L,8,"Unknown");
    
    lua_pushstring(L,val_to_str(val, (value_string*)(vs->data), def));
    
    return 1;
}

static int ValueString_gc(lua_State* L) {
    ValueString vs = checkValueString(L,1);
    
    g_array_free(vs,TRUE);
    
    return 0;
}

static int ValueString_tostring(lua_State* L) {
    ValueString vs = checkValueString(L,1);
    value_string* c = (value_string*)vs->data;
    GString* s = g_string_new("ValueString:\n");
    
    for(;c->strptr;c++) {
        g_string_sprintfa(s,"\t%u\t%s\n",c->value,c->strptr);
    }
    
    lua_pushstring(L,s->str);
    
    g_string_free(s,TRUE);
    
    return 1;
}

static const luaL_reg ValueString_methods[] = {
    {"new",   ValueString_new},
    {"add",   ValueString_add},
    {"match", ValueString_match},
    {0,0}
};


static const luaL_reg ValueString_meta[] = {
    {"__gc",       ValueString_gc},
    {"__tostring", ValueString_tostring},
    {0, 0}
};


extern int ValueString_register(lua_State* L) {
    luaL_openlib(L, VALUE_STRING, ValueString_methods, 0);
    luaL_newmetatable(L, VALUE_STRING);
    luaL_openlib(L, 0, ValueString_meta, 0);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pushliteral(L, "__metatable");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pop(L, 1);
    
    return 1;
}







