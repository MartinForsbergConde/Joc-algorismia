#include "Player.hh"


/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME Intelligent2


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
    typedef priority_queue<IP> PCua;
    typedef priority_queue<IIP> QCua;


    //Guarden les millors direccions a seguir per arribar a una CITY o PATH, i els costos del cam� que et diuen
    VVI best_dir;
    VVI best_cost;

    //Guardem un per�metre de security_radius al voltant dels orcs enemics amb valors de la vida m�xims amb que arribarien a aquella
    //posici�, si poden fer security_radius passes.
    int security_radius = 5;
    int escape_radius = 4;
    VVI vida_enemiga;
    VVI dist_enemic;

    //Per a l'intel�ligent, distancia a la que ha anat a parar l'ork despr�s de l'�ltim moviment que ha fet en la ronda anterior
    VI last_dist;

    // Vida que es perd en moure's a una casella del tipus t
    int perdua_vida(CellType t){
        if (t != CITY and t != PATH) return t; //Casualment el cost coincideix amb l'enter que els correspon,
                                                // excepte en el cas de la city i el path
        return 0;
    }

    // Direcci� oposada a dir, en enter
    int inversint(Dir dir) {
        if (dir == BOTTOM) return TOP;
        if (dir == RIGHT) return LEFT;
        if (dir == TOP) return BOTTOM;
        if (dir == LEFT) return RIGHT;
        return NONE;
    }

    // Assegura que l'ork que es mou all� sempre tindr� m�s o igual vida que un enemic que hi pugui arribar en security_radius torns.
    bool safe_dir(Unit& ork, Dir dir){
        Pos pos = ork.pos;
        if (last_dist[ork.id] >= dist_enemic[pos.i][pos.j] and dist_enemic[pos.i][pos.j] > 3) return true; //Sembla que no estem sent perseguits
        Pos desti = pos + dir;
        Cell c = cell(desti);
        if (dir == NONE) return ork.health >= vida_enemiga[pos.i][pos.j] + perdua_vida(c.type); //Sumem perdua_vida de la casella perqu� l'ork no hi haur� d'anar a la mateixa casella, oi?.
        return ork.health - 2*perdua_vida(c.type) >= vida_enemiga[desti.i][desti.j];
    }

    // D�na una direcci� que intenta aproparte de o a p
    // Falta millorar-la perqu� sempre et doni direccions v�lides. �s a dir, simplement s'aproxima, primer en l'eix vertical, despr�s en
    // l'horitzontal a p, si hi ha alg� o aigua en la direcci�, la retorna igualment.
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

    // Crea un camp de direccions que indica cap on �s la millor direcci� per apropar-se a un lloc segur
    // A m�s guarda el cost m�nim en vida d'arribar al lloc segur, que s'aconsegueix si s'escull el cam� marcat pel camp
    void create_the_force() {
        //El camp de direccions es comen�a a generar desde les ciutats (obs: tots els camins estaran connectats a ciutats)
        for (int k = 0; k < nb_cities(); ++k){
            City cy = city(k);
            PCua pq; //Recordem que quan fem top donen primer el m�xim, de manera que guardarem els costos en negatiu.
            Pos posi = cy[0];
            //Si la ciutat no havia estat explorada pr�viament des de la seva component connexa:
            if (best_cost[posi.i][posi.j] != 0){
                best_cost[posi.i][posi.j] = 0;
                IP costposi (0, posi); //Cost de vida i Posici� per on comencem a explorar
                pq.push(costposi);
            }
            while(not pq.empty()) {
                IP costpos = pq.top(); pq.pop(); // La posici� amb menys cost de vida que hem guardat a la cua
                Pos pos = costpos.second;
                int perdues = -costpos.first;
              //  Cell c = cell(pos);
              //  cerr << pos << ' ' << perdues << ' ' <<  c.type;
                //Comprovem que estem treient m�nim cost que hem guardat en best_cost que pot tenir aquella posici�, sin� l'eliminem sense fer res
                if (perdues == best_cost[pos.i][pos.j]){
                   // cerr << "�s bona ";
                    for (int d = 0; d != NONE; ++d){ //Explorem les direccions al voltant de la posici� de m�nim cost que hem tret de la cua
                        Dir dir = Dir(d);
                        Pos vei = pos + dir;
                        Cell c = cell(vei);
                        if (c.type != WATER){
                            int cost = perdues + perdua_vida(c.type); //Cost que segur que tindrem tenint en compte que hem vingut per pos
                            if (cost < best_cost[vei.i][vei.j]) {
                                best_cost[vei.i][vei.j] = cost;
                                IP costvei (-cost, vei);
                                pq.push(costvei);
                                if (cost == 0) best_dir[vei.i][vei.j] = NONE; //Aix� voldria dir que estic en un cam� o ciutat de la component connexa actual
                                else best_dir[vei.i][vei.j] = inversint(dir); //Perqu� els orcs aniran cap a les ciutats,
                                //em guardo com anar a pos des de vei
                            }
                        }
                    }
                }
            //    cerr << endl;
            }
        }
    }

    // Crea un camp de valors amb a cada punt la vida m�xima a que hi pot arribar un ork enemic, amb el radi de moviment especificat
    void create_life_camp(){
        //Recorrem per a tots els orks
        for (int k = 0; k < nb_units(); ++k){
            Unit ork = unit(k);
            if (ork.player != me()){ //Que siguin enemics
                Pos posi = ork.pos;
                IP distposi(0, ork.pos);
                IIP vidadatai(ork.health, distposi);
                QCua qq; //Fem un dijkstra des d'ell, fitat per security_radius.
                qq.push(vidadatai);
                dist_enemic[posi.i][posi.j] = 0; //Ser� l'�ptim en dist�ncia
                if (vidadatai.first > vida_enemiga[posi.i][posi.j]) vida_enemiga[posi.i][posi.j] = vidadatai.first;

                while(not qq.empty()){
                    IIP vidadata = qq.top(); qq.pop(); //Treiem de la cua la posici� amb vida m�s gran, que ja ser� la �ptima, o una dolenta ignorable
                    Pos pos = (vidadata.second).second;
                    int dist = -(vidadata.second).first; //Guardarem la dist�ncia en negatiu, perqu� ens interessa petita.

                    if ((vida_enemiga[pos.i][pos.j] == vidadata.first or dist_enemic[pos.i][pos.j] == dist) and dist < security_radius){
                        //Per a entrar comprovem que �s l'�ptim en vida o en dist�ncia, i que no �s ja a la frontera
                        for (int d = 0; d < NONE; ++d) {
                            Dir dir = Dir(d);
                            Pos vei = pos + dir;
                            Cell c = cell(vei);
                            if (c.type != WATER){
                                int vida_max = vidadata.first - perdua_vida(c.type);
                                int vei_dist = dist + 1;
                                bool optim_vida = vida_max > vida_enemiga[vei.i][vei.j];
                                bool optim_dist = vei_dist < dist_enemic[vei.i][vei.j];
                                if (optim_vida){
                                    //Hem trobat un nou �ptim de vida enemiga
                                    vida_enemiga[vei.i][vei.j] = vida_max; //Si la vida de l'ork passa a ser negativa, deixem d'explorar i no arribem aqu�.
                                }
                                if (optim_dist){
                                    //Hem trobat un nou �ptim de dist�ncia enemiga
                                    dist_enemic[vei.i][vei.j] = dist + 1;
                                }
                                if (optim_vida or optim_dist) {
                                    //Guardem el nou �ptim per a explorar-lo futurament
                                    IP distpos(-vei_dist, vei);
                                    IIP vidavei(vida_max, distpos);
                                    qq.push(vidavei);
                                }
                            }
                        }
                    }
                }
            }

        }
    }

    // Busca la direcci� que t'apropa millor a una CITY o un PATH, o que et mogui per dins seu
    Dir exclusive_nearest_home(Pos pos){
        Cua q; // L'usarem per a la cerca en amplada
        VVB visitat = VVB(rows(), VB(cols(),0));
        VVP pare = VVP(rows(), VP(cols()));

        q.push(pos);
        visitat[pos.i][pos.j] = 1;
        while(not q.empty()){
            Pos act = q.front(); q.pop();
            VI perm = random_permutation(4); // Faig que s'explorin les caselles ve�nes aleat�riament
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
                        q.push(vei); //Guardo aquest ve� que no �s ni segur ni �s aigua.
                        pare[vei.i][vei.j] = act;
                        visitat[vei.i][vei.j] = 1;
                    }
                }
            }
        }
        return Dir(NONE); //No ha trobat cap casella segura
    }

    // Busca la direcci� que t'apropa millor a una CITY o un PATH
    Dir nearest_home(Pos pos) {
        Cell c = cell(pos);
        if (c.type == CITY or c.type == PATH) return Dir(NONE); // Ja erem en una casella segura

        return exclusive_nearest_home(pos);
    }

    // Si existeix, troba la direcci� que porta cap al lloc sense conquerir m�s proper, sense perdre vida
    Dir easy_conquer_from(Pos pos) {
        //Per a trobar el cam� m�s curt, fem una b�squeda en amplada
        Cua q;
        VVB visitat = VVB(rows(), VB(cols(),0));
        VVP pare = VVP(rows(), VP(cols()));

        q.push(pos);
        visitat[pos.i][pos.j] = 1;
        while(not q.empty()){
            Pos act = q.front(); q.pop();
            VI perm = random_permutation(4); // Faig que s'explorin les caselles ve�nes aleat�riament
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
                            // a por �l
                            while(act != pos){
                                vei = act;
                                act = pare[act.i][act.j];
                            }
                            return a_por_ellos(vei, pos);
                        }
                        //O, continuo per aquest ve� que �s cam� o ciutat meva:
                        q.push(vei);
                        pare[vei.i][vei.j] = act;
                        visitat[vei.i][vei.j] = 1;
                    }
                }
            }
        }
        return Dir(NONE); //Tota la component connexa �s nostra :')
    }

    // Si existeix, troba la direcci� que porta cap al lloc sense conquerir m�s proper, sense perdre vida i sense apropar-se a enemics xetats
    Dir safe_easy_conquer_from(Unit& ork) {
        //Per a trobar el cam� m�s curt, fem una b�squeda en amplada
        Cua q;
        VVB visitat = VVB(rows(), VB(cols(),0));
        VVP pare = VVP(rows(), VP(cols()));
        Pos pos = ork.pos;
        q.push(pos);
        visitat[pos.i][pos.j] = 1;
        while(not q.empty()){
            Pos act = q.front(); q.pop();
            VI perm = random_permutation(4); // Faig que s'explorin les caselles ve�nes aleat�riament
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
                                // a por �l
                                while(act != pos){
                                    vei = act;
                                    act = pare[act.i][act.j];
                                }
                                return a_por_ellos(vei, pos);
                            }
                            //O, continuo per aquest ve� que �s cam� o ciutat meva:
                            q.push(vei);
                            pare[vei.i][vei.j] = act;
                            visitat[vei.i][vei.j] = 1;
                        }
                    }
                }
            }
        }
        return Dir(NONE); //Tota la component connexa �s nostra :') o ens hem cagat :,(
    }

    // If in home (city or path) returns NONE.
    // Returned direction dir moves to a safe cell (criterion: safe_dir()) with less cost to reach home, if it doesn't exist,
    // stays in place if it is safe. Otherwise, returns DIR_SIZE, meaning there is no safe way home.
    Dir safe_way_home(int ork_id) {
        //Busquem el cam� m�s proper amb m�nim cost, i d'aquests el que l'enemic t� menys vida
        Unit ork = unit(ork_id);
        Cell c = cell(ork.pos);
        if (c.type == CITY or c.type == PATH) return Dir(NONE); //No cal perqu� ja ho faria l'�ltim if per� estalviem computaci�?

        int minim_cost = best_cost[(ork.pos).i][(ork.pos).j];
        int minim_enemic = vida_enemiga[(ork.pos).i][(ork.pos).j];
        Dir best_safe_dir = Dir(DIR_SIZE); //Si no ho hem canviat abans del final de la funci� �s que no hi ha cam� en direcci� cap a casa i segur

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

    // Escapa allunyant-se de l'enemic m�s proper, i despr�s cap al lloc on tenen menys vida
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
    // Posible millora: a m�s, que sigui un cam� de m�nim cost.
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
                VI perm = random_permutation(4); // Faig que s'explorin les caselles ve�nes aleat�riament
                for (int d = 0; d != NONE; ++d){
                    Dir dir = Dir(perm[d]);
                    Pos vei = act + dir;
                    if (distancia[vei.i][vei.j] == -1){ // No l'havia visitat
                        Cell c = cell(vei);
                        if (c.type != WATER) {
                            //O, continuo per aquest ve� que �s acessible:
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
        //Amb el seg�ent anem a l'�ptim que hem trobat.
        Pos to_optim = optim;
        while(to_optim != pos){
            optim = to_optim;
            to_optim = pare[optim.i][optim.j];
        }
        return a_por_ellos(optim, pos);

    }

  /**
   * Play method, invoked once per each round.
   */
  virtual void play () {
    VI my_orks = orks(me()); //Id dels meus orks

    if (round() == 0) {
        best_dir = VVI(rows(), VI(cols(), NONE));
        best_cost = VVI(rows(), VI(cols(), 100)); //M�nim cost en vida per tornar a una ciutat/cam� si anem a la cell(i,j).
        //Inicialitzem en 100 perqu� �s la vida dels orks i si �s m�s gran que aix� no podran arribar all� on sigui.
        create_the_force();

        last_dist = VI(nb_units(),140); //En posar un nombre gran considerem que a la primera ronda mai som perseguits. O primera vegada que guanyo control de l'ork.
    }

    //Sempre:
    vida_enemiga = VVI(rows(), VI(cols(), -1));
    dist_enemic = VVI(rows(), VI(cols(), 140));
    create_life_camp();

    if (round() == 199) {
        double st = status(me());
        cerr << "Cost cpu: " << st << endl;
    }

    //Decidim que far� cada ork
    for(int k = 0; k < int (my_orks.size()); ++k){
        Unit ork = unit(my_orks[k]);
        Pos pos = ork.pos;

        Dir dir = safe_way_home(ork.id); //If in home (city or path) returns NONE.
        //Returned direction dir moves to a safe cell (criterion: safe_dir()) with less cost to reach home, if it doesn't exist,
        //stays in place if it is safe. Otherwise, returns DIR_SIZE, meaning there is no safe way home.

        Cell c = cell(pos);
        if (c.type == CITY or c.type == PATH){
            //L'ork es troba en una CITY o PATH
            dir = safe_easy_conquer_from(ork);
            if (dir == NONE){
                //Tota la component connexa �s conquerida, tota? S�, tota.
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
