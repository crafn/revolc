# revolc
Rewrite of the clover engine. The name has a symbolic meaning, as now everything is done the opposite compared to the original engine, i.e. the right way, not the wrong way.

This will be written in plain C instead of C++, as C makes it easy to write simple and efficient code. My only worry is templates, as they're hard to replace in C. I'll not try to replace them, but instead (somehow) generate the required stuff, which produces simpler and easier to debug code, and won't hurt compile times. Void pointers can be used where sufficient.

I'm also looking forward to the upcoming programming language created by Jonathan Blow, and choosing C should make the possible porting trivial.

Here's some things which might help understanding my viewpoint.

## Things that went wrong with the clover engine

##### Compile times
Building the clover engine takes 5 minutes. Rebuilding after changing a few .cpp's often takes 10-20s. This is too long for rapid development, discourages making important changes to header files, and decreases motivation.

Solution: No more templates. No more C++ standard library.

##### Efficiency
Memory allocations and cache efficiency are very hard to fix afterwards in a large object-oriented system. These caused major slowdown, and most of them could've been prevented easily beforehand with correct mindset.

Solution: No more objects encapsulating allocations. Data oriented thinking. Simplicity over "elegancy", "maintainability" and "robustness"; don't believe what _they_ say, just make it simple. Future self will thank you.

##### Resources/assets
The old resource system is overly complicated, as it tries to deal with resources which can unload and reload themselves on the fly, and act correctly when some other resources they are using are reloaded with different data. Also, resources are in a human readable format (JSON), which makes startup of the game very slow and painful.

Solution: Reduce complication by making resources memcpyable so that human readable files can be preprocessed to a single binary blob, which can be straightforwardly memcpy'd to the resouce structs. Removing one-to-one correspondence between human readable stuff and engine structures is a good thing because then we don't have to take major performance hit for stuff like `entity->model->material->texture->gl_id`.

#### Rendering
The rendering system has actually just evolved from my first one. I think that explains.

##### Gui
The old gui system is buggy and bloated. Not sure what to do about it. I'd like to try a non-object-oriented approach.

##### Game objects
In principle, I like the flexibility of the node system in creating game objects. In practice, the implementation is overly complex and slow, which is largely due to OOP principles. I've hidden the actual logic under a pile of abstraction (objects), which makes it really inconvenient to create nodes out of e.g. simple functions. It also makes hard to re-use the nodes in ordinary code as they're tightly bound to the node system.

Solution: Implement nodes with structs and functions which operate on an array of nodes. Generate fast and type-safe glue code.

##### Third party libraries
Just say no to boost and similar things. Using bloated libraries will induce large amounts of pain afterwards. Everybody's code is buggy, but it's a lot better if the bugs are in a code that is simple and does exactly the thing needed, i.e. your own code.

Solution: Write the code by yourself, or use minimalistic libraries. 

##### Fancy language features
I'd like to be able to compile on MSVC. I don't like gcc crashing during compilation.

Solution: No new standards. Plain, ordinary and simple code.

## Things that went quite good

##### Audio system
This was already rewritten some time ago. It's a bit too complicated, but hasn't caused any problems so far.

##### Animation system
Again, the object oriented layers over efficient data structures cause complications, but the core procedures should be fine.

##### Removing the script system
Reloading DLL's at runtime is a lot simpler.

For further info, I've written something on my website:

http://crafn.kapsi.fi/?path=news/clover_engine_mistakes/

http://crafn.kapsi.fi/?path=clover/pain_in_game_development/
