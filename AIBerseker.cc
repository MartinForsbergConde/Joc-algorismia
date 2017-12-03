#include "Player.hh"


/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME Berseker


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
    typedef priority_queue<IP> PCua;

    /**
    *Faltarà saber com anar d'una component connexa a una altra
    */
    //Guarden les millors direccions a seguir per arribar a una CITY o PATH, i els costos del camí que et diuen
    VVI best_dir;
    VVI best_cost;

    //Ens diu si la cel·la és segura (de moment, segura vol dir no cost en vida)
    bool safe_cell(Cell c){
        return c.type == CITY or c.type == PATH;
    }

    //Vida que es perd en moure's a una casella del tipus t
    int perdua_vida(CellType t){
        if (t != CITY and t != PATH) return t; //Casualment el cost coincideix amb l'enter que els correspon,
                                                // excepte en el cas de la city i el path
        return 0;
    }

    //Direcció oposada a dir, en enter
    int inversint(Dir dir) {
        if (dir == BOTTOM) return TOP;
        if (dir == RIGHT) return LEFT;
        if (dir == TOP) return BOTTOM;
        if (dir == LEFT) return RIGHT;
        return NONE;
    }

    //Dóna una direcció que intenta aproparte de o a p
    //Falta millorar-la perquè sempre et doni direccions vàlides. És a dir, simplement s'aproxima, primer en l'eix vertical, després en
    //l'horitzontal a p, si hi ha algú o aigua en la direcció, la retorna igualment.
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
              //  Cell c = cell(pos);
              //  cerr << pos << ' ' << perdues << ' ' <<  c.type;
                //Comprovem que estem treient mínim cost que hem guardat en best_cost que pot tenir aquella posició, sinó l'eliminem sense fer res
                if (perdues == best_cost[pos.i][pos.j]){
                   // cerr << "és bona ";
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
            //    cerr << endl;
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

    //Si existeix, troba la direcció que porta cap al lloc sense conquerir més proper, sense perdre vida
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

    }
    if (round() == 199) {
        double st = status(me());
        cerr << "Cost cpu: " << st << endl;
    }

    //Decidim que farà cada ork
    for(int i = 0; i < int (my_orks.size()); ++i){
        Unit ork = unit(my_orks[i]);
        Pos pos = ork.pos;
        Dir dir = Dir(best_dir[pos.i][pos.j]);
        if (dir == NONE){
            //L'ork es troba en una CITY o PATH
            dir = easy_conquer_from(pos);
            if (dir == NONE){
                //Tota la component connexa és conquerida, tota? Sí, tota.
                dir = exclusive_nearest_home(pos);
            }
        }
        execute(Command(ork.id, dir));
    }
  }

};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);


//./Game Petito Mitjanet Mitjanet Mitjanet -s 350 -i default.cnf -o default.out
