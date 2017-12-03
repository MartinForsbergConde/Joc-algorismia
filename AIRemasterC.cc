#include "Player.hh"


/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME RemasterC

//Millora del guardat de matriu d'info sobre els enemics.

struct PLAYER_NAME : public Player {

  /**
   * Factory: returns a new instance of this class.
   * Do not modify this function.
   */
  static Player* factory () {
    return new PLAYER_NAME;
  }

  /**
   * Types and attributes for your player can be defined here.
   */
    typedef vector<int> VI;
    typedef vector<VI> VVI;
    typedef vector<bool> VB;
    typedef vector<VB> VVB;
    typedef vector<Pos> VP;
    typedef vector<VP> VVP;
    typedef queue<Pos> Cua;
    typedef pair<int, Pos> IP;
    typedef pair<int, int> II;
    typedef pair<int, IP> IIP;
    typedef pair<int, II> III;
    typedef priority_queue<IP> PCua;
    typedef priority_queue<IIP> QCua;
    typedef vector<III> VIII;
    typedef vector<VIII> VVIII;
    typedef vector<VVIII> HDL; //Servirà per guardar llistes de health, distància i id


    //Guarden les millors direccions a seguir per arribar a una CITY o PATH, i els costos del camí que et diuen
    VVI best_dir;
    VVI best_cost;

    //Guardem un perímetre de security_radius al voltant dels orcs enemics amb valors de la vida màxims amb que arribarien a aquella
    //posició, si poden fer security_radius passes.
    int security_radius = 5;
    int danger_radius = 3; // Moure's a una casella a aquesta distància o menys d'un enemic que ens hi pot matar, mai es considera segur.
    int escape_radius = 4;
    VVI vida_enemiga;
    VVI dist_enemic;
    VVIII auxiliary_enemy_max_health;
    VVIII auxiliary_enemy_min_dist;
    HDL enemy_max_health; // Guarda sobre un ork enemic la màxima vida amb que pot arribar a una casella i quants moviments li costa
    HDL enemy_min_dist;// Guarda sobre un ork enemic la mínima distància amb que pot arribar a una casella i quanta vida li costa
    int list_size = 3; // Quanta informació sobre orks ens podem guardar com a molt en cada casella de la llista.
    //OBS les distàncies les guardem en negatiu.

    //Per a l'intel·ligent, distancia a la que ha anat a parar l'ork després de l'últim moviment que ha fet en la ronda anterior
    VI last_dist;

    //Per a millorar eficiència en trobar el lloc no conquerit més proper amb moviment restringit(només CITY i PATH). Guarda la distància
    //més curta a lloc sense conquerir amb moviment restringit.
    VVI dist_conquesta;
    VVI dir_conquesta;

    // Vida que es perd en moure's a una casella del tipus t
    int perdua_vida(CellType t){
        if (t != CITY and t != PATH) return t; //Casualment el cost coincideix amb l'enter que corresopn al terreny, excepte en la city i el path
        return 0;
    }

    // Direcció oposada a dir, en enter
    int inversint(Dir dir) {
        if (dir == BOTTOM) return TOP;
        if (dir == RIGHT) return LEFT;
        if (dir == TOP) return BOTTOM;
        if (dir == LEFT) return RIGHT;
        return NONE;
    }

    // Ens diu que si arribem a una posició amb una certa vida, si ningú ens hi podrà matar (potser sí, per empat)
    bool pos_segura(Pos pos, int health_at_pos){
        return health_at_pos >= vida_enemiga[pos.i][pos.j] + perdua_vida((cell(pos)).type);
        //Sumem perdua_vida de la casella perquè l'ork no hi haurà d'anar a la mateixa casella, oi?.
    }

    // Ens diu que si en moure'ns a una posició i teníem una certa vida, si ningú ens hi podrà matar (potser sí, per empat)
    bool mov_segur(Pos pos, int health_at_pos){
        return health_at_pos >= vida_enemiga[pos.i][pos.j] + 2*perdua_vida((cell(pos)).type);
    }

    //Intenta deduïr si som perseguits a partir del què fan els orks enemics
    bool perseguit(int ork_id, Pos pos){
        return last_dist[ork_id] < dist_enemic[pos.i][pos.j]; //És a dir, l'enemic s'ha apropat
    }

    // Assegura que l'ork que es mou allí sempre tindrà més o igual vida que un enemic que hi pugui arribar en security_radius torns.
    // Si es considera que està sent perseguit o està a distància més gran que danger_radius.
    bool safe_dir(Unit& ork, Dir dir){
        Pos pos = ork.pos;
        Pos desti = pos + dir;
        if (not perseguit(ork.id, pos) and dist_enemic[desti.i][desti.j] > danger_radius) return true; //No ens hauria de passar res
        //Si estem sota possible amenaça:
        if (dir == NONE) return pos_segura(pos, ork.health);
        return mov_segur(desti, ork.health);
    }

    // Dóna una direcció que intenta aproparte de o a p
    // Falta millorar-la perquè sempre et doni direccions vàlides. És a dir, simplement s'aproxima, primer en l'eix vertical, després en
    // l'horitzontal a p, si hi ha algú o aigua en la direcció, la retorna igualment.
    Dir a_por_ellos(Pos p, Pos o){
        if (p.i != o.i) {
            if (p.i > o.i) return Dir(0);
            else return Dir(2);
        }
        else if (p.j != o.j) {
            if (p.j > o.j) return Dir(1);
            else return Dir(3);
        }
        return Dir(4);
    }

    // Crea un camp de direccions que indica cap on és la millor direcció per apropar-se a un lloc segur
    // A més guarda el cost mínim en vida d'arribar al lloc segur, que s'aconsegueix si s'escull el camí marcat pel camp
    void create_the_force() {
        //El camp de direccions es comença a generar desde les ciutats (obs: tots els camins estaran connectats a ciutats)
        for (int k = 0; k < nb_cities(); ++k){
            City cy = city(k);
            PCua pq; //Recordem que quan fem top donen primer el màxim, de manera que guardarem els costos en negatiu.
            Pos posi = cy[0];
            //Si la ciutat no havia estat explorada prèviament des de la seva component connexa:
            if (best_cost[posi.i][posi.j] != 0){
                best_cost[posi.i][posi.j] = 0;
                IP costposi (0, posi); //Cost de vida i Posició per on comencem a explorar
                pq.push(costposi);
            }
            while(not pq.empty()) {
                IP costpos = pq.top(); pq.pop(); // La posició amb menys cost de vida que hem guardat a la cua
                Pos pos = costpos.second;
                int perdues = -costpos.first;
                //Comprovem que estem treient mínim cost que hem guardat en best_cost que pot tenir aquella posició, sinó l'eliminem sense fer res
                if (perdues == best_cost[pos.i][pos.j]){
                    for (int d = 0; d != NONE; ++d){ //Explorem les direccions al voltant de la posició de mínim cost que hem tret de la cua
                        Dir dir = Dir(d);
                        Pos vei = pos + dir;
                        Cell c = cell(vei);
                        if (c.type != WATER){
                            int cost = perdues + perdua_vida(c.type); //Cost que segur que tindrem tenint en compte que hem vingut per pos
                            if (cost < best_cost[vei.i][vei.j]) {
                                best_cost[vei.i][vei.j] = cost;
                                IP costvei (-cost, vei);
                                pq.push(costvei);
                                if (cost == 0) best_dir[vei.i][vei.j] = NONE; //Això voldria dir que estic en un camí o ciutat de la component connexa actual
                                else best_dir[vei.i][vei.j] = inversint(dir); //Perquè els orcs aniran cap a les ciutats,
                                //em guardo com anar a pos des de vei
                            }
                        }
                    }
                }
            }
        }
    }

    //Insertem un òptim que hem trobat en la matriu enemy_max_health
    void insert_max_health(Pos pos, III optim) {
        for (int k = 0; k < list_size; ++k){
            if (optim > enemy_max_health[pos.i][pos.j][k]) swap(enemy_max_health[pos.i][pos.j][k], optim);
            //Obs, guardem les distàncies en negatiu, perquè volem que siguin el més petites possible en valor absolut
        }
    }

    //Insertem un òptim que hem trobat en la matriu de enemy_min_dist
    void insert_min_dist(Pos pos, III optim) {
        for (int k = 0; k < list_size; ++k){
            if (optim > enemy_min_dist[pos.i][pos.j][k]) swap(enemy_min_dist[pos.i][pos.j][k], optim);
            //Obs, guardem les distàncies en negatiu, perquè volem que siguin el més petites possible en valor absolut
        }
    }

    // Crea dos camps de vectors amb a cada punt les més importants list_size valors de vida, distància i identitat dels orks enemics.
    // És a dir omple les matrius enemy_max_health i enemy_min_dist amb els valors que toca aquesta ronda.
    void create_life_camp(){
        //Recorrem per a tots els orks
        for (int k = 0; k < nb_units(); ++k){
            Unit ork = unit(k);
            if (ork.player != me()){ //Que siguin enemics
                Pos posi = ork.pos;
                //Ara hem d'omplir les dues matrius d'optimalitat per a aquest ork, amb els valors òtpims que tenim:
                III health_optim(ork.health, II(0, k));
                III dist_optim(0, II(ork.health, k));
                // que són segurs perquè estem en la primera posició de l'ork:
                auxiliary_enemy_max_health[posi.i][posi.j] = health_optim;
                auxiliary_enemy_min_dist[posi.i][posi.j] = dist_optim;

                //Comencem els dos Dijkstres:
                IP distposi(0, ork.pos); // La distànncia anirà en negatiu perquè la volem minimitzar
                IIP vidadatai(ork.health, distposi); // La cua optimitzarà respecte a la VIDA i després la distància
                QCua qq; //Fem un dijkstra des d'ell, fitat per security_radius.
                qq.push(vidadatai);
                while(not qq.empty()){
                    IIP vidadata = qq.top(); qq.pop();
                    Pos pos = (vidadata.second).second;
                    int dist = -(vidadata.second).first; //Guardarem la distància en negatiu, perquè ens interessa petita.
                    III optim = auxiliary_enemy_max_health[pos.i][pos.j];

                    if (optim.first == vidadata.first and -(optim.second).first == dist){ //Comprovem que no estem traient basura,
                    //segur que l'ork id és correcta per coses que farem després, només posarem un element a la QCua després d'haver modificat la matriu auxiliar d'òptims
                        insert_max_health(pos, optim); //Introduïm l'òptim segur a la matriu definitiva enemy_max_health
                        if (dist < security_radius) //Mirem els veins d'aquest òptim definitiu.
                        for (int d = 0; d < NONE; ++d) {
                            Dir dir = Dir(d);
                            Pos vei = pos + dir;
                            Cell c = cell(vei);
                            if (c.type != WATER){
                                int vida_max = vidadata.first - perdua_vida(c.type);
                                int vei_dist = dist + 1;
                                III nou_optim(vida_max, II(-vei_dist, k));
                                III antic_optim = auxiliary_enemy_max_health[vei.i][vei.j];
                                if ((antic_optim.second).second != k or nou_optim > antic_optim){
                                    //És a dir, no havíem visitat aquesta casella per a aquest ork o sí que ho havíem fet, per un camí pitjor
                                    auxiliary_enemy_max_health[vei.i][vei.j] = nou_optim;
                                    IP distvei(-vei_dist, vei);
                                    IIP vidavei(vida_max, distvei);
                                    qq.push(vidavei);
                                }
                            }
                        }
                    }
                }

                IP vidaposi(ork.health, ork.pos); // La distància anirà en negatiu perquè la volem minimitzar
                IIP distdatai(0, vidaposi); // La cua optimitzarà respecte la distància i després respecte la vida (buscant la màxima en el segon cas)
                //Fem un dijkstra des d'ell, fitat per security_radius.
                qq.push(distdatai);
                while(not qq.empty()){
                    IIP distdata = qq.top(); qq.pop();
                    Pos pos = (distdata.second).second;
                    int dist = -distdata.first; //Guardarem la distància en negatiu, perquè ens interessa petita.
                    III optim = auxiliary_enemy_min_dist[pos.i][pos.j];

                    if (optim.first == distdata.first and (optim.second).first == (distdata.second).first){ //Comprovem que no estem traient basura,
                    //segur que l'ork id és correcta per coses que farem després, només posarem un element a la QCua després d'haver modificat la matriu auxiliar d'òptims
                        insert_min_dist(pos, optim); //Introduïm l'òptim segur a la matriu definitiva enemy_min_dist
                        if (dist < security_radius) //Mirem els veins d'aquest òptim definitiu.
                        for (int d = 0; d < NONE; ++d) {
                            Dir dir = Dir(d);
                            Pos vei = pos + dir;
                            Cell c = cell(vei);
                            if (c.type != WATER){
                                int vida_max = (distdata.second).first - perdua_vida(c.type);
                                int vei_dist = dist + 1;
                                III nou_optim(-vei_dist, II(vida_max, k));
                                III antic_optim = auxiliary_enemy_min_dist[vei.i][vei.j];
                                if ((antic_optim.second).second != k or nou_optim > antic_optim){
                                    //És a dir, no havíem visitat aquesta casella per a aquest ork o sí que ho havíem fet, per un camí pitjor
                                    auxiliary_enemy_min_dist[vei.i][vei.j] = nou_optim;
                                    IP vidavei(vida_max, vei);
                                    IIP distvei(-vei_dist, vidavei);
                                    qq.push(distvei);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Crea un camp de valors amb a cada punt la vida màxima a que hi pot arribar un ork enemic, amb el radi de moviment especificat
    // I un altre amb la distància mínima amb que pot arribar un enemic
    void create_old_matrices(){
        //Recorrem per a totes les posicions
        for (int i = 0; i < rows(); ++i){
            for (int j = 0; j < cols(); ++j){
                    //Si un és no buit, l'altre també? NO
                if ((enemy_min_dist[i][j][0]).first != -140){
                    dist_enemic[i][j] = -(enemy_min_dist[i][j][0]).first;
                    if ((enemy_max_health[i][j][0]).first != -1) vida_enemiga[i][j] = (enemy_max_health[i][j][0]).first;
                    else if (((enemy_min_dist[i][j][0]).second).first >= 0)
                        vida_enemiga[i][j] = ((enemy_min_dist[i][j][0]).second).first;
                }
            }
        }
    }

    //Crea des de 0 (de fet des de 140) un camp on posa la distància menor a un lloc no conquerit per dins de CITY o PATH
    void create_conquer_camp () {
        dist_conquesta = VVI(rows(), VI(cols(), 140)); //Valor impossible d'arribar inicialment.
        for (int k = 0; k < nb_cities(); ++k){
            if (city_owner(k) != me()){ //Explorem des de les ciutats que no són meves.
                City cy = city(k);
                Cua q;
                Pos posi = cy[0];
                if (dist_conquesta[posi.i][posi.j] != 0){ //No havíem visitat la ciutat no conquerida
                    dist_conquesta[posi.i][posi.j] = 0;
                    q.push(posi); //L'explorem
                    while(not q.empty()){
                        Pos pos = q.front(); q.pop(); //Explorem el que hem tret de la cua
                        int dist = dist_conquesta[pos.i][pos.j];
                        for (int d = 0; d != NONE; ++d){ //Visitem els veïns.
                            Dir dir = Dir(d);
                            Pos vei = pos + dir;
                            Cell c = cell(vei);
                            if (c.type == PATH or c.type == CITY){ //Ens movem per lloc restringit
                                int owner;
                                if (c.type == CITY) owner = city_owner(c.city_id);
                                if (c.type == PATH) owner = path_owner(c.path_id);
                                int new_dist;
                                if (owner != me()) new_dist = 0; //He trobat una nova casella d'un lloc no conquerit
                                else new_dist = dist + 1;
                                if (new_dist < dist_conquesta[vei.i][vei.j]){ //He trobat una manera millor
                                    q.push(vei);
                                    dist_conquesta[vei.i][vei.j] = new_dist;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Busca la direcció que t'apropa millor a una CITY o un PATH, o que et mogui per dins seu
    Dir exclusive_nearest_home(Pos pos){
        Cua q; // L'usarem per a la cerca en amplada
        VVB visitat = VVB(rows(), VB(cols(),0));
        VVP pare = VVP(rows(), VP(cols()));

        q.push(pos);
        visitat[pos.i][pos.j] = 1;
        while(not q.empty()){
            Pos act = q.front(); q.pop();
            VI perm = random_permutation(4); // Faig que s'explorin les caselles veïnes aleatòriament
            for (int d = 0; d != NONE; ++d){
                Dir dir = Dir(perm[d]);
                Pos vei = act + dir;
                if (pos_ok(vei) and not visitat[vei.i][vei.j]){
                    Cell c = cell(vei);
                    if (c.type != WATER){
                        if (c.type == CITY or c.type == PATH) {
                            while(act != pos){
                                vei = act;
                                act = pare[act.i][act.j];
                            }
                            return a_por_ellos(vei,pos);
                        }
                        q.push(vei); //Guardo aquest veí que no és ni segur ni és aigua.
                        pare[vei.i][vei.j] = act;
                        visitat[vei.i][vei.j] = 1;
                    }
                }
            }
        }
        return Dir(NONE); //No ha trobat cap casella segura
    }

    // Busca la direcció que t'apropa millor a una CITY o un PATH
    Dir nearest_home(Pos pos) {
        Cell c = cell(pos);
        if (c.type == CITY or c.type == PATH) return Dir(NONE); // Ja erem en una casella segura

        return exclusive_nearest_home(pos);
    }

    // Si existeix, troba la direcció que porta cap al lloc sense conquerir més proper, sense perdre vida
    Dir easy_conquer_from(Pos pos) {
        //Per a trobar el camí més curt, fem una búsqueda en amplada
        Cua q;
        VVB visitat = VVB(rows(), VB(cols(),0));
        VVP pare = VVP(rows(), VP(cols()));

        q.push(pos);
        visitat[pos.i][pos.j] = 1;
        while(not q.empty()){
            Pos act = q.front(); q.pop();
            VI perm = random_permutation(4); // Faig que s'explorin les caselles veïnes aleatòriament
            for (int d = 0; d != NONE; ++d){
                Dir dir = Dir(perm[d]);
                Pos vei = act + dir;
                if (not visitat[vei.i][vei.j]){
                    Cell c = cell(vei);
                    if (c.type == CITY or c.type == PATH) {
                        int owner;
                        if (c.type == CITY) owner = city_owner(c.city_id);
                        if (c.type == PATH) owner = path_owner(c.path_id);
                        if (owner != me()){
                            // a por él
                            while(act != pos){
                                vei = act;
                                act = pare[act.i][act.j];
                            }
                            return a_por_ellos(vei, pos);
                        }
                        //O, continuo per aquest veí que és camí o ciutat meva:
                        q.push(vei);
                        pare[vei.i][vei.j] = act;
                        visitat[vei.i][vei.j] = 1;
                    }
                }
            }
        }
        return Dir(NONE); //Tota la component connexa és nostra :')
    }

    // Si existeix, troba la direcció que porta cap al lloc sense conquerir més proper, sense perdre vida i sense apropar-se a enemics xetats
    Dir safe_easy_conquer_from(Unit& ork) {
        //Per a trobar el camí més curt, fem una búsqueda en amplada
        Cua q;
        VVB visitat = VVB(rows(), VB(cols(),0));
        VVP pare = VVP(rows(), VP(cols()));
        Pos pos = ork.pos;
        q.push(pos);
        visitat[pos.i][pos.j] = 1;
        while(not q.empty()){
            Pos act = q.front(); q.pop();
            VI perm = random_permutation(4); // Faig que s'explorin les caselles veïnes aleatòriament
            for (int d = 0; d != NONE; ++d){
                Dir dir = Dir(perm[d]);
                if (safe_dir(ork, dir)){
                    Pos vei = act + dir;
                    if (not visitat[vei.i][vei.j]){
                        Cell c = cell(vei);
                        if (c.type == CITY or c.type == PATH) {
                            int owner;
                            if (c.type == CITY) owner = city_owner(c.city_id);
                            if (c.type == PATH) owner = path_owner(c.path_id);
                            if (owner != me()){
                                // a por él
                                while(act != pos){
                                    vei = act;
                                    act = pare[act.i][act.j];
                                }
                                return a_por_ellos(vei, pos);
                            }
                            //O, continuo per aquest veí que és camí o ciutat meva:
                            q.push(vei);
                            pare[vei.i][vei.j] = act;
                            visitat[vei.i][vei.j] = 1;
                        }
                    }
                }
            }
        }
        return Dir(NONE); //Tota la component connexa és nostra :') o ens hem cagat :,(
    }

    //Busca una direcció segura per a moure's baixant pel gradient de distància a les conquestes tant com pugui
    Dir safe_conquest_approach(Unit ork){
        Dir safe_conq_dir = Dir(DIR_SIZE);
        int min_dist = 140; //Només em podré moure dins de ciutat perquè les caselles no ciutat tindran aquest valor de cost
        VI perm = random_permutation(5);
        for (int d = 0; d != DIR_SIZE; ++d){
            Dir dir = Dir(perm[d]);
            if (safe_dir(ork, dir)){
                Pos vei = ork.pos + dir;
                if (dist_conquesta[vei.i][vei.j] < min_dist) {
                    min_dist = dist_conquesta[vei.i][vei.j];
                    safe_conq_dir = dir;
                }
            }
        }
        return safe_conq_dir;
    }

    // If in home (city or path) returns NONE.
    // Returned direction dir moves to a safe cell (criterion: safe_dir()) with less cost to reach home, if it doesn't exist,
    // stays in place if it is safe. Otherwise, returns DIR_SIZE, meaning there is no safe way home.
    Dir safe_way_home(int ork_id) {
        //Busquem el camí més proper amb mínim cost, i d'aquests el que l'enemic té menys vida
        Unit ork = unit(ork_id);
        Cell c = cell(ork.pos);
        if (c.type == CITY or c.type == PATH) return Dir(NONE); //No cal perquè ja ho faria l'últim if però estalviem computació?

        int minim_cost = best_cost[(ork.pos).i][(ork.pos).j]; //No acceptem un cost pitjor o igual que el que ja tenim
        int minim_enemic = vida_enemiga[(ork.pos).i][(ork.pos).j];
        Dir best_safe_dir = Dir(DIR_SIZE); //Si no ho hem canviat abans del final de la funció és que no hi ha camí en direcció cap a casa i segur

        for (int d = 0; d != NONE; ++d){
            Dir dir = Dir(d);
            if (safe_dir(ork, dir)) {
                Pos vei = ork.pos + dir;
                if (best_cost[vei.i][vei.j] < minim_cost){
                    minim_cost = best_cost[vei.i][vei.j];
                    minim_enemic = vida_enemiga[vei.i][vei.j];
                    best_safe_dir = Dir(d);
                }
                if (best_cost[vei.i][vei.j] == minim_cost){
                    if (minim_enemic > vida_enemiga[vei.i][vei.j]){
                        minim_enemic = vida_enemiga[vei.i][vei.j];
                        best_safe_dir = Dir(d);
                    }
                }
            }
        }

        if (minim_cost == best_cost[(ork.pos).i][(ork.pos).j]) {
            if(safe_dir(ork, Dir(NONE))) best_safe_dir = Dir(NONE);
            //Si no hem millorat de cost, encara que haguem trobat un lloc on els enemics tenen menys vida, ens quedem quiets si podem.
        }

        return best_safe_dir;
    }

    // Escapa allunyant-se de l'enemic més proper, i després cap al lloc on tenen menys vida
    Dir fuig(Pos pos) {
        II minim_enemic(-dist_enemic[pos.i][pos.j], vida_enemiga[pos.i][pos.j]); //Baixem pel gradient de la -dist, vida enemiga
        Dir dir_fugida = Dir(NONE);
        for (int d = 0; d != NONE; ++d) {
            Dir dir = Dir(d);
            Pos vei = pos + dir;
            if (cell(vei).type != WATER){
                II vei_enemic(-dist_enemic[vei.i][vei.j], vida_enemiga[vei.i][vei.j]);
                if (vei_enemic < minim_enemic) {
                    dir_fugida = dir;
                }
            }
        }
        return dir_fugida;
    }

    Dir fuig_aprop_ciutat(Pos pos){
        II lluny_enemic(-dist_enemic[pos.i][pos.j], best_cost[pos.i][pos.j]); //Baixem pel gradient de la -dist, vida enemiga
        Dir dir_fugida = Dir(NONE);
        for (int d = 0; d != NONE; ++d) {
            Dir dir = Dir(d);
            Pos vei = pos + dir;
            if (cell(vei).type != WATER){
                II vei_enemic(-dist_enemic[vei.i][vei.j], best_cost[vei.i][vei.j]);
                if (vei_enemic < lluny_enemic) {
                    dir_fugida = dir;
                }
            }
        }
        return dir_fugida;
    }

    // Anar a la casella amb menys cost per arribar a una ciutat que sigui segura, en menys de safaety_radius?
    // Posible millora: a més, que sigui un camí de mínim cost.
    Dir planeja_fugida(Unit ork) {
        Cua q;
        VVI distancia = VVI(rows(), VI(cols(),-1));
        VVP pare = VVP(rows(), VP(cols()));
        Pos pos = ork.pos;
        q.push(pos);
        distancia[pos.i][pos.j] = 0;
        Pos optim = pos;
        int minim_cost_segur = 100;
        while(not q.empty()){
            Pos act = q.front(); q.pop();
            int dist = distancia[act.i][act.j];
            if (dist < escape_radius){
                VI perm = random_permutation(4); // Faig que s'explorin les caselles veïnes aleatòriament
                for (int d = 0; d != NONE; ++d){
                    Dir dir = Dir(perm[d]);
                    Pos vei = act + dir;
                    if (distancia[vei.i][vei.j] == -1){ // No l'havia visitat
                        Cell c = cell(vei);
                        if (c.type != WATER) {
                            //O, continuo per aquest veí que és acessible:
                            q.push(vei);
                            pare[vei.i][vei.j] = act;
                            distancia[vei.i][vei.j] = dist + 1;
                            if (dist_enemic[vei.i][vei.j] > security_radius and minim_cost_segur > best_cost[vei.i][vei.j]){
                                minim_cost_segur = best_cost[vei.i][vei.j];
                                optim = vei;
                            }
                        }
                    }
                }
            }
        }
        //Amb el següent anem a l'òptim que hem trobat.
        Pos to_optim = optim;
        while(to_optim != pos){
            optim = to_optim;
            to_optim = pare[optim.i][optim.j];
        }
        return a_por_ellos(optim, pos);

    }

    // Mira al seu voltant si hi ha un ork amb menor vida i l'ataca
    Dir ataca_entorn(Unit ork){
        for(int d = 0; d != NONE; ++d){
            Dir dir = Dir(d);
            Pos vei = ork.pos + dir;
            if (vida_enemiga[vei.i][vei.j] < ork.health and dist_enemic[vei.i][vei.j] == 0){
                return dir;
            }
        }
        return NONE;
    }

  /**
   * Play method, invoked once per each round.
   */
  virtual void play () {
    VI my_orks = orks(me()); //Id dels meus orks

    if (round() == 0) {
        best_dir = VVI(rows(), VI(cols(), NONE));
        best_cost = VVI(rows(), VI(cols(), 100)); //Mínim cost en vida per tornar a una ciutat/camí si anem a la cell(i,j).
        //Inicialitzem en 100 perquè és la vida dels orks i si és més gran que això no podran arribar allí on sigui.
        create_the_force();

        last_dist = VI(nb_units(),140); //En posar un nombre gran considerem que a la primera ronda mai som perseguits. O primera vegada que guanyo control de l'ork.
    }

    //Sempre:
    auxiliary_enemy_max_health = VVIII(rows(), VIII(cols(), III(-1, II(-140,-1))));
    auxiliary_enemy_min_dist = VVIII(rows(), VIII(cols(), III(-140, II(-1,-1))));
    enemy_max_health = HDL(rows(), VVIII(cols(), VIII(list_size, III(-1, II(-140,-1)))));
    enemy_min_dist = HDL(rows(), VVIII(cols(), VIII(list_size, III(-140, II(-1,-1)))));
    create_life_camp();
    vida_enemiga = VVI(rows(), VI(cols(), -1));
    dist_enemic = VVI(rows(), VI(cols(), 140));
    create_old_matrices();
    create_conquer_camp();

    if (round() == 199) {
        double st = status(me());
        cerr << "Cost cpu: " << st << endl;
    }

    //Decidim que farà cada ork
    for(int k = 0; k < int (my_orks.size()); ++k){
        Unit ork = unit(my_orks[k]);
        Pos pos = ork.pos;
        Dir dir = ataca_entorn(ork);
        if (dir == NONE) dir = safe_way_home(ork.id); //If in home (city or path) returns NONE.
        //Returned direction dir moves to a safe cell (criterion: safe_dir()) with less cost to reach home, if it doesn't exist,
        //stays in place if it is safe. Otherwise, returns DIR_SIZE, meaning there is no safe way home.

        Cell c = cell(pos);
        if ((c.type == CITY or c.type == PATH) and dir == NONE){
            //L'ork es troba en una CITY o PATH
            dir = safe_conquest_approach(ork);
            if (dir == NONE){
                //Tota la component connexa és conquerida, tota? Sí, tota.
                dir = exclusive_nearest_home(pos);//Es mou random per dins
                if(not safe_dir(ork, dir)) dir = Dir(DIR_SIZE);
            }
        }

        if (dir == DIR_SIZE){
            dir = planeja_fugida(ork);
            if (dir == NONE) {
                dir = fuig_aprop_ciutat(ork.pos);
                //cerr <<"Estic fugint esbojarradament! " << ork.id << endl;
                }
        }

        execute(Command(ork.id, dir));

        //Guardem coses sobre on s'ha mogut l'ork
        Pos desti = pos + dir;
        last_dist[my_orks[k]] = dist_enemic[desti.i][desti.j];
    }
  }

};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);


//./Game Petito Mitjanet Mitjanet Mitjanet -s 350 -i default.cnf -o default.out
