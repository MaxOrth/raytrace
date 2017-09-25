


class Mutex
{
  void Aquire() = 0;
  void Release() = 0;
  // assignment operator? or just specialize swap? or move...
};

template <typename T>
class LockPtr
{
 private:
  Mutex &mutex;
  T *obj;
public:
  LockPtr(T volatile &objref, Mutex &mtx)
   : mutex(mtx), obj(const_cast<T*>(&objref))
  {
    mutex.Aquire();
  }
  
  // maybe?
  //LockPtr(T volatile *objref, Mutex *mtx)
  // : mutex(*mtx), obj(const_cast<T*>(objref))
  //{}
  
  ~LockPtr()
  {
    mutex.Release();
  }
  
  LockPtr &operator=(LockPtr &&rhs)
  {
    mutex = std::move(rhs.mutex);
    obj = rhs.obj;
    rhs.obj = nullptr;
  }
  
  T& operator*()
  {
    return *obj;
  }
  T *operator->()
  {
    return obj;
  }

  LockPtr(LockPtr const &) = delete;
  LockPtr& operator=(LockPtr const &) = delete;
};

template <typename T>
LockPtr<T> lvptr(T volatile &obj)
{
  return LockPtr(obj, obj.Mutex());
}

template <typename T>
LockPtr<T> lvptr(T volatile &obj, Mutex &mtx)
{
  return LockPtr(obj, mtx);
}


