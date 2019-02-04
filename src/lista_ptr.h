#pragma once

#include <stddef.h>

typedef void (*FuncDeletar)(void*);

typedef struct _ListaPtr ListaPtr;

struct _ListaPtr {
        void        **dados;
        FuncDeletar fun_deletar;
        size_t      item_tamanho;
        size_t      total;
};

ListaPtr* lista_ptr_new            (size_t item_tamanho);

void      lista_ptr_free           (ListaPtr *lista);

void      lista_ptr_set_fun_del    (ListaPtr      *lista,
                                    FuncDeletar  fun_del);

void*     lista_ptr_posicao        (const ListaPtr *lista,
                                    size_t       pos);

void      lista_ptr_inserir        (ListaPtr *lista,
                                    void   *ponteiro);

void      lista_ptr_inserir_inicio (ListaPtr *lista,
                                    void   *ponteiro);
