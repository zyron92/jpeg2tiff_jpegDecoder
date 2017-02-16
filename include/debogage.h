#ifndef __DEBOGAGE_H__
#define __DEBOGAGE_H__

#define LOCALISATION  "[file %s, line %d]: "
#define LOCALARG  __FILE__, __LINE__
#define ERREUR(...) fprintf(stderr, __VA_ARGS__)
#define ERREUR1(_msg) ERREUR(LOCALISATION _msg, LOCALARG)
#define ERREUR2(_msg, ...) ERREUR(LOCALISATION _msg, LOCALARG, __VA_ARGS__)

#endif
