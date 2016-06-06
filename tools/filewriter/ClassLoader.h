/*
 * ClassLoader.h
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_CLASSLOADER_H_
#define TOOLS_FILEWRITER_CLASSLOADER_H_

#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>

#define REGISTER(Base,Class,Name) filewriter::ClassLoader<Base> cl(Name, filewriter::maker<Base,Class>);

namespace filewriter
{
  /**
   * Function template to instantiate a class.
   * It returns a shared pointer to its base class
   */
  template <typename BaseClass, typename SubClass> boost::shared_ptr<BaseClass> maker()
  {
    boost::shared_ptr<BaseClass> ptr = boost::shared_ptr<BaseClass>(new SubClass);
    return ptr;
  }

  /**
   * C++ dynamic class loader.
   */
  template <typename BaseClass> class ClassLoader
  {
    typedef boost::shared_ptr<BaseClass> maker_t();

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
     */
    static boost::shared_ptr<BaseClass> load_class(const std::string& name, const std::string& path)
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
      boost::shared_ptr<BaseClass> obj;
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

} /* namespace filewriter */


#endif /* TOOLS_FILEWRITER_CLASSLOADER_H_ */
