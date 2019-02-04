#pragma once

#include <stddef.h>

typedef void (*FuncDeletar)(void*);

typedef struct _ListaP ListaP;

struct _ListaP {
        void        **dados;
        FuncDeletar fun_deletar;
        size_t      item_tamanho;
        size_t      total;
};

ListaP* lista_p_new            (size_t item_tamanho);

void    lista_p_free           (ListaP *lista);

void    lista_p_set_fun_del    (ListaP      *lista,
                                FuncDeletar  fun_del);

void*   lista_p_posicao        (const ListaP *lista,
                                size_t       pos);

void    lista_p_inserir        (ListaP *lista,
                                void   *ponteiro);

void    lista_p_inserir_inicio (ListaP *lista,
                                void   *ponteiro);
