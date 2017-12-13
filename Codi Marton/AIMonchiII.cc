#include "Player.hh"
#include "Utils.hh"
using namespace std;




#define PLAYER_NAME MonchiII

struct PLAYER_NAME : public Player {

  static Player* factory () {
    return new PLAYER_NAME;
  }

  typedef vector<int> VI; typedef vector<double> VF; typedef vector<bool> VB; typedef vector<Pos> VP; typedef vector<Dir> VD;
  typedef vector<VI> VVI; typedef vector<VF> VVF; typedef vector<VB>  VVB; typedef vector<VP>  VVP; typedef vector<VD> VVD;
  typedef vector<VVI> VVVI; typedef vector<VVF> VVVF; typedef vector<VVB>  VVVB; typedef vector<VVP>  VVVP; typedef vector<VVD> VVVD;

  float dis(int a1, int a2, int b1, int b2) {
    return float(abs(a1 - b1)*abs(a2 - b2))/(10*float(abs(a1-b1) + abs(a2-b2)));
  } 

  float dis(Pos pos1, Pos pos2) {
    return dis(pos1.i, pos1.j, pos2.i, pos2.j);
  }

  int threshhold = 85, meanHP;
  VI my_orks, other_orks;
  VVVI dstrong_en; 
  vector<vector<bool> > taken;

  bool water_next(Pos pos) {
    for (int d = 0; d <= 4; ++d) {
      Pos nxt = pos + Dir(d);
      Cell c = cell(nxt.i, nxt.j);
      if (c.type == WATER) return true;
    }
    return false;
  }

  struct casilla {
    int hp;
    int dis;
    Pos pos;
    int water;
    bool operator<(const casilla& rhs) const { 
      if (hp != rhs.hp) return hp < rhs.hp; 
      if (dis != rhs.dis) return dis > rhs.dis;
      return water > rhs.water;
    }
  };


  //return true if unit(i) has more health than unit(j).
  bool more_health(int i, int j) {
    Unit u = unit(i);
    Unit v = unit(j);
    return u.health > v.health;
  }


  //The following quicksort is implemented just because the std does not work on my PC for some reason.
  void swap(int &a, int &b) {
    int t = a;
    a = b;
    b = t;
  }
  //Quicksort algorithm for sorting orcs (std::sort no em funcava)
  int partition (VI &arr, int low, int high) {
    int pivot = arr[high];    // pivot
    int i = (low - 1);  // Index of smaller element
    for (int j = low; j <= high - 1; j++) {
      if (more_health(arr[j], pivot)) {
        i++;
        swap(arr[i], arr[j]);
      }
    }
    swap(arr[i + 1], arr[high]);
    return (i + 1);
  }
  void quickSort(VI &arr, int low, int high) {
    if (low < high) {
      int pi = partition(arr, low, high);
      quickSort(arr, low, pi - 1);
      quickSort(arr, pi + 1, high);
    }
  }

  float maximum(float a, float b) { return ((a + b) + abs(a - b))/2;}
  float minimum(float a, float b) { return ((a + b) - abs(a - b))/2;}

  //Return cost in health of walking into some position
  int cost(Pos pos) {
    Cell c = cell(pos.i, pos.j);
    if (c.type == GRASS) return cost_grass();
    if (c.type == FOREST) return cost_forest();
    if (c.type == SAND) return cost_sand();
    if (c.type == CITY) return cost_city();
    if (c.type == PATH) return cost_path();
    if (c.type == WATER) return 1e6;
    return 1e7;
  }

  bool no_water(Pos pos) {
    Cell c = cell(pos.i, pos.j);
    if (c.type == WATER) return false;
    return true;
  }

  void bfs_write_h(Pos pos, int hp) {  //Dijkstra to write dstrong_en
    VVI max_hp(rows(), VI(cols(), -100));
    max_hp[pos.i][pos.j] = hp;
    priority_queue<casilla> q;
    casilla inicial = {hp, 0, pos, 0};
    q.push(inicial);
    while(!q.empty()) { 
      casilla a = q.top();
      q.pop();
      Pos fr = a.pos;
      for (int d = 0; d < 4; ++d) {
        Pos nxt = fr + Dir(d);
        if (no_water(nxt) and max_hp[nxt.i][nxt.j] < max_hp[fr.i][fr.j] - cost(nxt)) {
          max_hp[nxt.i][nxt.j] = max_hp[fr.i][fr.j] - cost(nxt);
          if (dstrong_en[nxt.i][nxt.j][max_hp[nxt.i][nxt.j]] > a.dis) {
            casilla pushed = {max_hp[nxt.i][nxt.j], a.dis + 1, nxt, 0};
            q.push(pushed);
            for (int i = max_hp[nxt.i][nxt.j]; i >= 0; --i) dstrong_en[nxt.i][nxt.j][i] = a.dis + 1;
          }
        }
      }
    }
  }

  void update() {
    taken = vector<vector<bool> >(rows(), vector<bool>(cols(), false));
    other_orks = VI(0);
    dstrong_en = VVVI(rows(), VVI(cols(), VI(initial_health() + 1, 1e6)));
    for (int i = 0; i < 4; ++i) {
      if (i != me()) {
        VI iorks = orks(i);
        for (int j = 0; j < int(iorks.size()); ++j) 
          other_orks.push_back(iorks[j]);
      }
    }
    meanHP = 0;
    for (int i = 0; i < int(other_orks.size()); ++i) {
      Unit u = unit(other_orks[i]);
      meanHP += u.health;
    }
    if (int(other_orks.size()) == 0) meanHP = 0;
    else meanHP /= int(other_orks.size());


    quickSort(my_orks, 0, my_orks.size() - 1);
    quickSort(other_orks, 0, other_orks.size() - 1);

    //Let's fill dstrong_en.
    int it = 0;

    for (int hp = initial_health() - 1; hp >= 0; --hp) {
      while (it < int(other_orks.size()) and (unit(other_orks[it])).health > hp) {
        bfs_write_h((unit(other_orks[it])).pos, hp);
        ++it;
      }
    }
  }

  bool can_move(Pos pos, int id) {
    if (taken[pos.i][pos.j]) return false;
    Cell c = cell(pos.i, pos.j);
    if (c.type == WATER) return false;
    if (c.unit_id != -1) {
      Unit u = unit(c.unit_id);
      Unit mine = unit(id);
      if (u.player == me()) return false;
      if (u.health > mine.health - cost(pos)) return false;
    }
    return true;
  }

  Dir dijk(Pos init, int id) {

    Unit mine = unit(id);

    //If we are next to an enemy we can kill, we do.
    for (int k = 0; k < 4; ++k) { 
      Pos nxt = init + Dir(k);
      Cell c = cell(nxt.i, nxt.j);
      if (c.unit_id != -1) {
        Unit u = unit(c.unit_id);
        if (u.player != me() and u.health < mine.health - cost(nxt) and not taken[nxt.i][nxt.j]) {
          bool go = true;
          for (int d = 0; d < 4; ++d) {
            Pos nnxt = nxt + Dir(d);
            Cell cnxt = cell(nnxt.i, nnxt.j);
            if (cnxt.unit_id != -1 and cnxt.unit_id != me() and cnxt.unit_id != c.unit_id and mine.health < (unit(cnxt.unit_id)).health) go = false;
          }
          if (go)  {
            return Dir(k);
          }
        }
      }
    }

    //If predators very close, we run to the SAFEST, then CHEAPEST, spot. PANIC
    if (dstrong_en[init.i][init.j][mine.health] <= 2 or (mine.health < threshhold and dstrong_en[init.i][init.j][mine.health] <= 3 )) { 
      float maxdis = 0;
      for (int d = 0; d < 4; ++d) {
        Pos nxt = init + Dir(d);
        if (can_move(nxt, id)){
          maxdis = maximum(maxdis, dstrong_en[nxt.i][nxt.j][max(0, mine.health - cost(nxt))]);
        }
      }
      int costmin = 1e6;
      for (int d = 0; d < 4; ++d) {
        Pos nxt = init + Dir(d);
        if (dstrong_en[nxt.i][nxt.j][max(0, mine.health - cost(nxt))] == maxdis and can_move(nxt, id)) {
          costmin = min(costmin, cost(nxt));
        }
      }
      int rand = random(0, 3);
      for (int d = 0; d < 4; ++d) {
        Pos nxt = init + Dir((d + rand)%4);
        if (dstrong_en[nxt.i][nxt.j][max(0, mine.health - cost(nxt))] == maxdis and can_move(nxt, id) and costmin == cost(nxt)) {
          return Dir((d + rand)%4);
        }
      }
    }

    //We take shortest path to KILL an orc losing little health. (Dijkstra)

    VVI max_hp(rows(), VI(cols(), -10));
    max_hp[init.i][init.j] = mine.health;
    priority_queue<casilla> q;
    casilla inicial = {(unit(id)).health, 0, init, 0};
    q.push(inicial);
    VVD shortest(rows(), VD(cols(), NONE));

    while(!q.empty()) { 
      casilla a = q.top();
      q.pop();
      Pos fr = a.pos;
      Cell c = cell(fr.i, fr.j);

      if (c.unit_id != -1 and c.unit_id != me() and max_hp[fr.i][fr.j] > (unit(c.unit_id)).health){
        bool go = true;
        for (int d = 0; d < 4; ++d) {
          Pos nxt = fr + Dir(d);
          Cell cnxt = cell(nxt.i, nxt.j);
          if (cnxt.unit_id != -1 and cnxt.unit_id != me() and max_hp[nxt.i][nxt.j] + cost(nxt) < (unit(cnxt.unit_id)).health) go = false;
        }
        if (go)  {
          return shortest[fr.i][fr.j];
        }
      }

      for (int d = 0; d < 4; ++d) {
        Pos nxt = fr + Dir(d);
        if (can_move(nxt, id) and max_hp[nxt.i][nxt.j] < max_hp[fr.i][fr.j] - cost(nxt) and a.dis < 15) {
          max_hp[nxt.i][nxt.j] = max_hp[fr.i][fr.j] - cost(nxt);
          casilla pushed = {max_hp[nxt.i][nxt.j], a.dis + 1, nxt, a.water + water_next(nxt)};
          if (fr == init) { 
            if (dstrong_en[nxt.i][nxt.j][max(0, mine.health - cost(nxt))] > 2) { 
              q.push(pushed);
              shortest[nxt.i][nxt.j] = Dir(d);
            }
          }
          else {
            q.push(pushed);
            shortest[nxt.i][nxt.j] = shortest[fr.i][fr.j];
          }
        }
      }
    }

    //If we cannot kill inside cities, we find a CITY to go to. (Dijkstra)
 
    max_hp = VVI(rows(), VI(cols(), -100));
    max_hp[init.i][init.j] = mine.health;
    q.push(inicial);
    shortest = VVD(rows(), VD(cols(), NONE));

    while(!q.empty()) { 
      casilla a = q.top();
      q.pop();
      Pos fr = a.pos;
      Cell c = cell(fr.i, fr.j);

      if ((c.type == CITY and city_owner(c.city_id) != me()) or (c.type == PATH and path_owner(c.path_id) != me())) {
        return shortest[fr.i][fr.j];
      }

      int rand = random(0, 3);
      for (int d = 0; d < 4; ++d) {
        Pos nxt = fr + Dir((d + rand)%4);
        if (can_move(nxt, id) and max_hp[nxt.i][nxt.j] < max_hp[fr.i][fr.j] - cost(nxt) and a.dis < 45) {
          max_hp[nxt.i][nxt.j] = max_hp[fr.i][fr.j] - cost(nxt);
          casilla pushed = {max_hp[nxt.i][nxt.j], a.dis + 1, nxt, a.water + water_next(nxt)};
          if (fr == init) { 
            if (dstrong_en[nxt.i][nxt.j][mine.health - cost(nxt)] > 2) { 
              q.push(pushed);
              shortest[nxt.i][nxt.j] = Dir((d+rand)%4);
            }
          }
          else {
            q.push(pushed);
            shortest[nxt.i][nxt.j] = shortest[fr.i][fr.j];
          }
        }
      }
    }
    if (mine.health < 10) return Dir(random(0,3));
    return NONE;
  }

  void move(int id) {
    Unit u = unit(id);
    Pos pos = u.pos;
    Dir dir = dijk(pos, id);
    taken[(pos+dir).i][(pos+dir).j] = true;
    execute(Command(id, dir));
  }

  void move_lol(int id) {
    int r = round()%4;
    if (r%4 == 2) r = 0;
    Dir dir = Dir(r);
    execute(Command(id, dir));
  }

  virtual void play () {
    my_orks = orks(me());
    if ((status(me()) <= 1 and status(me()) >= 0.94) or 
      (round() > 170 and total_score(me()) > (total_score(0) + total_score(1) + total_score(2) + total_score(3))*2/3)) {
      for (int k = 0; k < int(my_orks.size()); ++k) 
        move_lol(my_orks[k]);
      return;
    }
    update();
    for (int k = 0; k < int(my_orks.size()); ++k) 
      move(my_orks[k]);
  }
};

RegisterPlayer(PLAYER_NAME);  