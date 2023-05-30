---
geometry: "left=1.5cm,right=1.5cm,top=1.5cm,bottom=1.5cm"
--- 



# The pandaPI Grounded HTN Format
This document describes the grounded HTN format used by the pandaPI planning system.
It is generated by the pandaPIgrounder and read by the pandaPIengine, which contains the actual planning algorithm.

The grounded HTN format is split into eleven section.
Each section is mandatory and is described in the sections below.
The order of the sections is fixed.

The grounded HTN format allows for line comments.
A line starting with the `;` character is considered a line commend.
Empty lines may be inserted into the input at any position.

## State Features
The first section describes the state features (in Fast Downwards naming scheme: facts) of the problem.
A state feature is a named boolean variable that can either be true or false in every state.
The first line of this section contains a single integer $F$ -- the number of state features.
Thereafter follow $F$ lines, each describing a state feature.
The $i$th state feature (0-indexed) is described in the $i+1$th line (i.e. state feature number $0$ in the first line).
Each state such line contains a single string which does not contain any whitespace characters -- the name of the state feature.

## Mutex Groups
The second sections describes the mutex groups of the problem.
Each mutex groups forms an SAS+ variable present in the problem.
This section starts with a line containing a single integer $S$ -- the number of mutex groups.
Thereafter follow $S$ lines.
Each such line describes one mutex group.
It contains two space-separated integers $f$ and $\ell$ followed by a space and a string that does not contain any whitespace characters. 
Provided that $f \neq \ell$, such a line indicates that the state features numbered $\{f,\dots,\ell\}$ form a mutex group, i.e. in any reachable state exactly one of them is true.
Note that the group *includes* $\ell$.

Consecutive mutex groups are always connected, i.e. $\ell+1$ for one group equals $f$ for the next group.
The first group always has $f = 0$ and for the last group $\ell$ is equal to $F-1$ (i.e. it will always be the last state feature).
As such, the mutex groups always cover all state features.


Note that mutex groups may have $f = \ell$.
In this case, the state feature $f$ does not belong to any mutex group in the SAS+ representation.
It can either be true or false in any state.
One can also interpret this as a two element SAS+ variable with the state feature $f$ and a new state feature named `not-`$f$.

## Further Strict Mutexes
The third section describes further known strict mutexes.
This section starts with a line containing a single integer $M$ -- the number of further strict mutexes.
Then follow $M$ lines, each describing a strict mutex.
Each such line contains a space-separated list of integers.
The last integer of the line is always $-1$ while all other integers are non-negative (i.e. you can read integers until you find a $-1$).


The integers in each line (except for the $-1$) describe a state feature, i.e.\ a line `0 1 -1` describes a strict mutex between the state features numbered $0$ and $1$.
Let -- for an arbitrary strict mutex -- these state features be $\{f_1, \dots, f_m\}$.
Then in any reachable state exactly one of the state features $f_1, \dots, f_m$ will be true.

Note that every mutex group with $f \neq l$ from the third section is a strict mutex group.
It however is not explicitly repeated in this section.


## Further Non-Strict Mutexes
The fourth section describes further known non strict mutexes.
This section has exactly the same format as the section describing further strict mutexes.
The only difference is that of the state features described by each line only *at most one* is true in every reachable state.
I.e. it might be possible that there is a reachable state in which none of the state features is true.

If a mutex group is known to be strict, it will only appear in the section for further strict mutex groups.

## Invariants
The fifth section describes further known state invariants.
This section starts with a line containing a single integer $I$ -- the number of known invariants.
Thereafter follow $I$ lines, each describing an invariant.


Each line describing an invariant contains a space-separated list of integers.
The last integer is always a $-1$ while no other integer is a $-1$.
Each integer except for the $-1$ describes either a state variable or the negation of a state variable (i.e. the fact that the state variable does not hold).
A non-negative integer $i$ refers to the state variable numbered $i$ (i.e. a $0$ to the state variable $0$, a $1$ to the state variable $1$, and so on).
A negative integer $i$ refers to the negation of the state variable numbered $-i-2$ (i.e. a $-2$ to the negation state variable $0$, a $-3$ to the negation of the state variable $1$, and so one).
As such, each line describes a set of literals $\{\ell_1, \dots, \ell_m\}$ over the state variables.
The invariant specifies that the formula $\ell_1 \vee \dots \ell_m$ holds in every reachable state.


Technically, every mutex group also constitutes a set of invariants (a mutex group $\{f_1, \dots, f_m\}$ is equivalent to all possibly binary invariants $\neg f_i \vee \neg f_j$ with $1 \leq i \neg j \leq m$; strict mutexes further imply $f_1 \vee \dots \vee f_m$).
Any invariant implied by any previously stated mutex group is not repeated in the invariant section.


## Actions
The sixth section describes the actions (also called primitive tasks).
This section starts with a line containing a single integer $A$ -- the number of actions.
Thereafter follow $4A$ lines.
Each block of $4$ lines describes an action.
Similar to the facts, the actions are numbered from $0$ to $A-1$.
Note that this section does not contain the action's names.
They are contained in the next section.


The first line of each action description contains a single integer -- the cost of that action.
The second line contains a space-separated list of integers.
The last integer is always $-1$ and all other integers are non-negative.
Each such non-negative integer $i$ describes that the state feature $i$ is a precondition of this action.
The third and fourth line per action have the same format.
They consist of a sequence of blocks of integers.
Each block starts with an integer $\ell$.
The line ends with block with $\ell = -1$ which carries no further semantics.
For each other block, $\ell+1$ non-negative integers follow after the value $\ell$.
Each such block describes a conditional effect of the action.
The first $\ell$ integers after the first integer $\ell$ of each block describe the conditions of this conditional effect.
Each such integer $i$ refers to the state feature $i$.
The last integer $e$ of each block describes the effect of the conditional effect.
In the third line of each action this is the effect of adding the state feature $e$, while in the fourth line, it is the effect of deleting the state feature $e$.
Note that there might be *two* space between the last integer of each block and the next start of a block.

As an example, consider the four line block
```
2
2 3 -1
0 1 1 3 4 -1
1 2 2 2 5 6 7 -1
```
This describes an action with cost $2$ and two preconditions: $2$ and $3$.
The third line consist of two blocks: `0 1` and `1 3 4`.
The first describes an unconditional add effect on the state feature $1$, i.e. this actions always the state feature $1$.
The second block describes the conditional effect that if the state feature $3$ holds, then the state feature $4$ is added.
The fourth line also contain two blocks: `1 2 2` and `2 5 6 7`.
The first represents the conditional delete effect that state feature $2$ is delete if it holds prior to executing this action.
The second represents the conditional delete effect that deletes the state feature $7$ if $5$ and $6$ hold prior to executing the action.

Note that add effects take precedence over deleting effects.
Delete effects of a state feature $i$ will not be present if an unconditional add effect on the state feature $i$ is present.
They may however be present if the adding effect on $i$ is conditional.

## Initial State
The seventh section describes the initial state.
It consists of a single line containing space separated integers.
The last integer is always $-1$ while any other integer is non-negative.
Each such non-negative integer $i$ indicates that the state feature $i$ holds in the initial state.
Any state feature that does not occur in this line is false in the initial state.

## Goal
The eight section describes the goal.
It has the same format as the section describing the initial state.
Each non-negative integer $i$ in the goal line states that the state feature $i$ must hold in the final state of a plan.
The truth of any state feature that is not mentioned in this line is irrelevant.

## Tasks and Tasks Names
The ninth section describes the tasks -- both the primitive and abstract ones.
This section starts with a line containing a single integer $T$ -- the total number of tasks in this problem.
This number includes both the primitive actions and the abstract tasks.
It will always be at least one plus the number of action.
This section then contains $n$ lines.


Let $A < T$ be the number of actions specified in the sixth section.
The next $A$ lines describe the actions of the problem.
Each such line starts with a `0` followed by a space and a string which does not contain any whitespace characters.
The string in the $i$th line contains the name of the $i$th action.

Thereafter $T-A$ lines follow.
The $j$th such line -- which will be the line number $i = j + A$ after the line containing $T$ overall contains a `1` followed by a space and a string which does not contain any whitespace characters.
Each such line describes an abstract task with the given string as its name.
The number of such an abstract task will be $i$.


Note that primitive action and abstract tasks have a common numbering space.
If the problem has $A$ primitive actions and $T$ tasks in total, the primitive actions will be numbered $0$ to $A-1$ while the abstract tasks will be numbered $A$ to $T-1$.


## Initial Abstract Task.
The tenth section describes the initial abstract task.
It is a single integer $t_i$ that states that the initial task is the task numbered $t_i$.
If you consider HTN planning problems to have an initial plan, this line means that the initial plan always contains exactly one tasks: the task $t_i$.

## Decomposition Methods
The eleventh section describes the decomposition methods.
This section starts with a line containing a single integer $D$ -- the number of decomposition methods.
Thereafter follow $4D$ lines, where each method is described by a block of $4$ consecutive lines.

The first line per decomposition method always contains a string without any whitespace character -- the name of the method.
The second line contains a single non-negative integer $a$ -- the abstract task this method decomposes.
The third line contains a space-separated list of integers.
The last integer is always $-1$, while all other integers are non-negative.
The $i$th (0-indexed) of these integers describes the $i$th subtask of this method.
The value of the $i$th integer indicates the task of this subtask.
The fourth line contains an odd number of integers.
The last integer is always $-1$, while the other integers are non-negative.
The number of non-negative integers is thus always even.
Hence this list of non-negative integers can be interpreted as sequence of pairs $\langle (o_1^-,o_1^+), \dots, (o_n^-,o_n^+) \rangle$.
Each such pair $(o_i^-,o_i^+)$ describes an ordering constraint on the methods subtasks.
More precisely, each such pair stipulates that the $o_i^-$th subtask must occur before the $o_i^+$th subtask.
No guarantees are made w.r.t. to the question whether the set of ordering constraints is transitively closed or whether it contains contains transitively implied ordering.



Consider the following example:
```
method_name
10
6 5 5 3 -1
0 1 0 2 2 3 1 3 -1
```
These four lines describe a method named `method_name`, which decomposes the task number $10$.
It has four subtasks: one instance of task $6$ (with subtask ID $0$), two instances of task $5$ (with subtask IDs $1$ and $2$ respectively), and one instance of task $3$ (with subtask ID $3$).
Further there are the following ordering constraints: task ID $0$ is ordered before ID $1$, $0$ is ordered before $2$, $2$ is ordered before $3$, and $1$ is ordered before $3$.
In all, the method allows for decomposing task $10$ into one instance of task $6$, followed by two instances of task $5$ in parallel (i.e. without any order between them), followed by one instance of task $3$.


