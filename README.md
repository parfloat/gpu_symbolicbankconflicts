### Symbolic identification of shared memory bank conflicts ###

This is the source code for the tool presented in the paper:

*Adrian Horga, Ahmed Rezine, Sudipta Chattopadhyay, Petru Eles, Zebo Peng, "Symbolic Identification of Shared Memory
Based Bank Conflicts for GPUs", Journal of Systems Architecture, 2022.*

The code is structured into 3 folders:

1. "Direct_approach" - the tool that uses the direct encoding for detecting bank conflicts
2. "On-demand_conflict-guided_approach" - the tool that uses the step-by-step encoding for detecting bank conflicts
3. "Programs" - the folder containing the applications tested

---

### How do I get set up? ###

The tools are based on [GKLEE](https://github.com/Geof23/Gklee). Follow the instructions on the [GKLEE](https://github.com/Geof23/Gklee)
page in order to set-up both of the versions (i.e., "Direct_approach" and "On-demand_conflict-guided_approach").

---

### Testing guidelines ###

The applications tested are in the folder "Programs".
Since the ""On-demand_conflict-guided_approach" is the faster of the two proposed approaches, the applications are set up for that version.

As an observation, "histogram256" has variable transactions when writing to shared memory. As opposed to reading from shared memory.
That means, for "histogram256", the tool needs to target "write" operations. For the moment, this needs to be done directly in the tool's code.
In "BankConflictsSideChannel::computeModel()", the "write" analysis part needs to be uncommented and the "read" commented out. The tool
needs to be recompiled after the changes.

---

### Bugs & issues

For reporting bugs or any other issues please contact [Adrian Horga](mailto:adrian.horga@liu.se).
