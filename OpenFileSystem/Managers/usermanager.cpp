#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include "usermanager.h"
#include "../Util/util.h"
using namespace std;

bool logged;
string user, password;


int login(string usuario, string password, string id)
{

    if (logged)
        throw Exception("sesion already active");

    // validar que exista usuario
    // validar que password corresponda

    return 0;
}

int logout()
{
    if (!logged)
        throw Exception("no sesion active");

    // logout

    user.clear();
    password.clear();
    logged = false;

    return 0;
}

int mkgrp(string name)
{
    if (!logged)
        throw Exception("no sesion active");

    if (user != "root")
        throw Exception("user has no permissions");

    // validar que no exista el nombre. El nombre debe ser case sensitive
    // make group

    return 0;
}

int rmgrp(string name)
{
    if (!logged)
        throw Exception("no sesion active");

    if (user != "root")
        throw Exception("user has no permissions");

    // validar que exista el grupo
    // remove group

    return 0;
}

int mkusr(string usr, string pwd, string grp)
{
    if (!logged)
        throw Exception("no sesion active");

    if (user != "root")
        throw Exception("user has no permissions");

    if (usr.length() > 10)
        throw Exception("parameter -usr non-valid, max. lenght = 10");

    if (pwd.length() > 10)
        throw Exception("parameter -pwd non-valid, max. lenght = 10");

    if (grp.length() > 10)
        throw Exception("parameter -grp non-valid, max. lenght = 10");

    // validar que exista el grupo
    // validar que no exista el usuario

    // make user    

    return 0;
}

int rmusr(string usr)
{
    if (!logged)
        throw Exception("no sesion active");

    if (user != "root")
        throw Exception("user has no permissions");

    // validar que exista el usuario
    // remove user

    return 0;
}
