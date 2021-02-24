/*
 * MIT License
 *
 * Copyright (c) 2021 Alex Vie (silvercircle@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

namespace smp {
    Testclass::Testclass(std::string const&& name)
    {
        this->my_name = name;
        std::cout << "A Testclass with the name " << this->my_name << " was created." << std::endl;
    }

    Testclass::Testclass(std::string const& name)
    {
        this->my_name = name;
        std::cout << "A Testclass with the name " << this->my_name << " was created." << std::endl;
    }

    Testclass::Testclass(std::string const& name, size_t age)
    {
        this->my_name = name;
        this->age = age;
        std::cout << "A Testclass with the name " << this->my_name << " and the age " << this->age << " was created." << std::endl;

    }

    Testclass::~Testclass()
    {
        std::cout << "The Testclass '" << this->my_name << "' is about to be destroyed." << std::endl;
    }

    [[maybe_unused]] void Testclass::greet()
    {
        std::cout << "Hi, I am an object of type Testlcass and my name is " << this->my_name << "." << std::endl;
    }

    void run()
    {
        std::string name("Affe");
        std::cout << "Running shared pointer tests" << std::endl;
        std::string s;
        s.assign("foo");
        {
            std::string name("Foobar");
            /*
             * std::make_unique<T> is a shortcut for creating a unique_ptr<T>
             * make_unique<T>(a, ...) will instantiate an object of type T and call a matching
             * constructor with the parameters.a, ...
             */
            std::unique_ptr<Testclass> bar = std::make_unique<Testclass>(std::string("hello"));
            std::unique_ptr<Testclass> bar2 = std::make_unique<Testclass>(name);
            name.assign("Deppen");
            std::unique_ptr<Testclass> bar1 = std::make_unique<Testclass>(name, 20);
            /*
             * both pointers go out of scope here. The objects, they hold, will be destroyed,
             * their destructors called properly.
             */
        }
    }
}
