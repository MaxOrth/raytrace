
#include "partition.h"

#include <algorithm>
#include "..\OpenCL-Headers\Affine.h"

void CreatePartitionTree(PartitionTree *dest, Model *models, unsigned modelcount, SceneObject *objs, unsigned obj_count)
{
  std::vector<unsigned> xlist(objs);
  std::vector<unsigned> ylist(objs);

  for (unsigned i = 0; i < obj_count)
  {
    xlist[i] = i;
    ylist[i] = i;
  }

  std::sort(xlist.begin(), xlist.end(), [&](unsigned l, unsigned r) -> bool
  {
    SceneObject *ol = objs[l];
    SceneObject *or = objs[r];
    Model *ml = models[ol.mi];
    Model *mr = models[or.mi];
    vec3 l = (ml->vol.a + ml->vol.b) * 0.5f;
    vec3 r = (mr->vol.a + mr->vol.b) * 0.5f;
    l = mult(ol->mat, l);
    r = mult(or->mat, r);
    return l.x > r.y;
  });

  std::sort(ylist.begin(), ylist.end(), [&](unsigned l, unsigned r) -> bool
  {
    SceneObject *ol = objs[l];
    SceneObject * or = objs[r];
    Model *ml = models[ol.mi];
    Model *mr = models[or .mi];
    vec3 l = (ml->vol.a + ml->vol.b) * 0.5f;
    vec3 r = (mr->vol.a + mr->vol.b) * 0.5f;
    l = mult(ol->mat, l);
    r = mult(or ->mat, r);
    return r.y < l.y;
  });

  std::vector<std::vector<unsigned>>

}
