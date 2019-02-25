#include <stdlib.h>
#include <string.h>

#include "lista_ptr.h"

ListaPtr*
lista_ptr_new (size_t item_tamanho)
{
  ListaPtr *lista = malloc (sizeof (ListaPtr));
  lista->total = 0;
  lista->dados = NULL;
  lista->fun_deletar = NULL;
  lista->item_tamanho = item_tamanho;

  return lista;
}

void
lista_ptr_free (ListaPtr *lista)
{
  if (lista == NULL) {
    return;
  }
  else if (lista->dados == NULL) {
    free (lista);
    return;
  }

  if (lista->fun_deletar != NULL) {
    for (int i = 0; i < lista->total; i++) {
      lista->fun_deletar (lista->dados[i]);
    }
  }
  else {
    for (int i = 0; i < lista->total; i++) {
      free (lista->dados[i]);
    }
  }

  free (lista->dados);
  free (lista);
}

void
lista_ptr_set_fun_del (ListaPtr     *lista,
                       FuncDeletar  fun_del)
{
  if (lista == NULL) {
    return;
  }

  lista->fun_deletar = fun_del;
}

void*
lista_ptr_posicao (const ListaPtr *lista,
                   size_t         pos)
{
  if (lista->dados == NULL || pos >= lista->total) {
    return NULL;
  }

  return lista->dados[pos];
}

void
lista_ptr_inserir (ListaPtr *lista,
                   void   *ponteiro)
{
  if (lista->total == 0) {
    if (lista->dados == NULL) {
      lista->dados = malloc (lista->item_tamanho*1);
    }

    lista->total += 1;
    lista->dados[lista->total-1] = ponteiro;
  }
  else {
    const int novo_tamanho = lista->item_tamanho * (lista->total+1);
    lista->dados = realloc (lista->dados, novo_tamanho);

    if (lista->dados != NULL) {
      lista->total += 1;
      lista->dados[lista->total-1] = ponteiro;
    }
    else {
      //realloc falhou
      lista->total = 0;
    }
  }
}

void
lista_ptr_inserir_inicio (ListaPtr *lista,
                          void     *ponteiro)
{
  if (lista->total == 0) {
    if (lista->dados == NULL) {
      lista->dados = malloc (lista->item_tamanho*1);
    }

    lista->total += 1;
    lista->dados[lista->total-1] = ponteiro;
  }
  else {
    const int antigo_tamanho = lista->item_tamanho * lista->total;
    const int novo_tamanho = lista->item_tamanho * (lista->total+1);
    lista->dados = realloc (lista->dados, novo_tamanho);

    if (lista->dados != NULL) {
      lista->total += 1;

      // move todos os itens da lista pra trás,
      // deixando o primeiro item disponível 
      memmove (&lista->dados[1], lista->dados, antigo_tamanho);
      lista->dados[0] = ponteiro;
    }
    else {
      //realloc falhou
      lista->total = 0;
    }
  }
}
