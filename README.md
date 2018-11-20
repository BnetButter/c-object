# c-object
object-oriented C

Supports:
1. Polymorphic methods resolved at runtime
2. Single inheritance
3. Multiple inheritance
      a. methods resolved via C3 linearization
4. Message forwarding
5. Class methods

Class instantiation
new(metaclass, classname, num_bases, [base1, ...], sizeof(struct a_class), docstring, [methods, ..., ...])
   method = [selector, tag, method, docstring]
