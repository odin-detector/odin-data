/*
 * ClassLoader.h
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#ifndef ODIN_DATA_CLASSLOADER_H_
#define ODIN_DATA_CLASSLOADER_H_

#include <dlfcn.h>
#include <cstdio>
#include <unistd.h>
#include <map>
#include <stdexcept>
#include <string>
#include <memory>

#define REGISTER(Base,Class,Name) OdinData::ClassLoader<Base> cl(Name, OdinData::maker<Base,Class>);

namespace OdinData
{
/**
 * Function template to instantiate a class.
 * It returns a shared pointer to its base class
 */
template <typename BaseClass, typename SubClass> std::shared_ptr<BaseClass> maker()
{
  std::shared_ptr<BaseClass> ptr = std::shared_ptr<BaseClass>(new SubClass);
  return ptr;
}

/**
 * C++ dynamic class loader.  Classes are loaded by calling the static method
 * load_class for the specific BaseClass type to load.  This loader will keep a
 * record of loaded classes and so if the same class is requested twice it can
 * simply create another instance of the (already loaded) class.  Name mangling
 * is dealt with by having each individual class register itself as soon as the
 * library is opened by calling the REGISTER macro, which creates an instance of
 * the ClassLoader object specific to that class and registers it with the map,
 * indexed by name.
 */
template <typename BaseClass> class ClassLoader
{
  /**
   * Shared pointer to the specified BaseClass
   */
  typedef std::shared_ptr<BaseClass> maker_t();

public:
  /**
   * Create an instance of the class loader.
   * \param[in] name - name of the class to load
   * \param[in] value - pointer to function returning the base class pointer
   */
  ClassLoader(std::string name, maker_t *value)
  {
    factory_map()[name] = value;
  }

  /**
   * Load a class given the class name
   * \param[in] name - name of class to load
   * \param[in] path - full path of the library to load
   */
  static std::shared_ptr<BaseClass> load_class(const std::string& name, const std::string& path)
  {
    // First do we already have this class loaded?
    if (!is_registered(name)){
      // The class is not registered so we need to load it

      // This requires two things, firstly some sort of classpath will be
      // required Secondly, the class will need to be compiled into a library
      // of the same name.
      //
      // We are also assuming the class will register the same name.

      // This will open the shared library and in doing so register
      void *dlib = dlopen(path.c_str(), RTLD_NOW);
      if (dlib == NULL) {
        // Throw a traceable exception with the dlerror as the reason
        throw std::runtime_error(dlerror());
      } else {
        // The class has now been loaded.  In doing so it has registered its
        // make function with the factory map ready to produce instances.
      }
    }
    std::shared_ptr<BaseClass> obj;
    try {
      if (factory_map().count(name)) {
        obj = factory_map()[name]();
      }
    } catch(const std::exception& ex) {
      // Most likely the requested class and loaded class do not match.
      // Nothing we can do so return a null shared ptr
    }
    return obj;
  }

  /**
   * Function to return a map of functions returning base class pointers.
   * The map is indexed by the name of the class that is loaded.
   * \return - map of functions returning base class pointers.
   */
  static std::map<std::string, maker_t *> &factory_map()
  {
    static std::map<std::string, maker_t *> factory;
    return factory;
  }

  /**
   * Check if a class of the given name is already registered with
   * the class loader.
   * \param[in] name - the name of the class
   * \return - true if the class is already registered
   */
  static bool is_registered(std::string name)
  {
    return factory_map().end() != factory_map().find(name);
  }
};

} /* namespace OdinData */
#endif /* ODIN_DATA_CLASSLOADER_H_ */
