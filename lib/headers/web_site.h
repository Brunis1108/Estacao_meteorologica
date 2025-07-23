
#ifndef WEB_SITE_H
#define WEB_SITE_H

struct st_dados{
    double temp;
    double umi;
    double press;
};
struct st_get_dados{
    char modo[16];
    double min[3];
    double max[3];
    double ajuste[3];
};
// Variável global que armazena a configuração dos níveis de água
extern struct st_dados dados;
extern struct st_get_dados get_dados;
// Função para inicializar o servidor web
void init_web_site();

#endif