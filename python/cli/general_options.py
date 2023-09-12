# general_options.py
import click 

class GeneralOptions:
    def __init__(self, s, seqtype, t, o, prefix, seed, safe, mem, runs, v, version, quiet, fconst, epsilon, T, threads_max):
        self.s = s
        self.seqtype = seqtype
        self.t = t
        self.o = o
        self.prefix = prefix
        self.seed = seed
        self.safe = safe
        self.mem = mem
        self.runs = runs
        self.v = v
        self.version = version
        self.quiet = quiet
        self.fconst = fconst
        self.epsilon = epsilon
        self.T = T
        self.threads_max = threads_max
        
def general_options(f):
    for option in [
        click.option("--s", type=str, help="Alignment files or directory"),
        click.option("--seqtype", type=str, help="Sequence type (BIN, DNA, AA, NT2AA, CODON, MORPH)"),
        click.option("--t", type=str, help="Starting tree"),
        click.option("--o", type=str, help="Outgroup taxon"),
        click.option("--prefix", type=str, help="Prefix for output files"),
        click.option("--seed", type=int, help="Random seed"),
        click.option("--safe", is_flag=True, help="Safe mode"),
        click.option("--mem", type=int, help="Maximum memory usage (MB)"),
        click.option("--runs", type=int, help="Number of independent runs"),
        click.option("--verbose", is_flag=True, help="Verbose mode"),
        click.option("--version", is_flag=True, help="Show version and exit"),
        click.option("--quiet", is_flag=True, help="Quiet mode"),
        click.option("--fconst", type=str, help="Fixed base frequencies"),
        click.option("--epsilon", type=float, help="Log-likelihood epsilon"),
        click.option("--T", type=int, help="Number of threads"),
        click.option("--threads-max", type=int, help="Maximum number of threads"),
    ]:
        f = option(f)
    return f