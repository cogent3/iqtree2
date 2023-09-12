# cli.py
import click
from trogon import command, option, TrogonCLI
from general_options import GeneralOptions
import iqtree2_functions as iq

cli = TrogonCLI()

@command(cli, "random-tree")
@option("--num-taxa", type=int, help="Number of taxa for the random tree")
@option("--branch-length-mode", type=str, help="Mode for generating branch lengths")
def random_tree(num_taxa, branch_length_mode, general_options: GeneralOptions):
    """Generate a random tree"""
    iq.random_tree(num_taxa, branch_length_mode, general_options)

from trogon import command, option, TrogonCLI
import iqtree2_functions as iq
from general_options import GeneralOptions, general_options

cli = TrogonCLI()

@command(cli, "random-tree")
@option("--num-taxa", type=int, help="Number of taxa for the random tree")
@option("--branch-length-mode", type=str, help="Mode for generating branch lengths")
@general_options
def random_tree(num_taxa: int, branch_length_mode: str, general_opts: GeneralOptions):
    """
    Generate a random tree using specified parameters.

    Parameters
    ----------
    num_taxa : int
        Number of taxa for the random tree.
    branch_length_mode : str
        Mode for generating branch lengths.
    general_opts : GeneralOptions
        An instance of GeneralOptions class containing all the general options.

    Returns
    -------
    None
    """
    iq.random_tree(num_taxa, branch_length_mode, general_opts)

@command(cli, "phylogenetic-analysis")
@option("--aln-file", type=str, help="Path to the alignment file")
@option("--partition-file", type=str, help="Path to the partition file")
@option("--tree-file", type=str, help="Path to the initial tree file")
@option("--out-prefix", type=str, help="Prefix for output files")
@option("--num-threads", type=int, help="Number of threads to use")
@option("--seed", type=int, help="Random seed")
@general_options
def phylogenetic_analysis(aln_file: str, partition_file: str, tree_file: str, out_prefix: str, num_threads: int, seed: int, general_opts: GeneralOptions):
    """
    Run phylogenetic analysis using specified parameters.

    Parameters
    ----------
    aln_file : str
        Path to the alignment file.
    partition_file : str
        Path to the partition file.
    tree_file : str
        Path to the initial tree file.
    out_prefix : str
        Prefix for output files.
    num_threads : int
        Number of threads to use.
    seed : int
        Random seed.
    general_opts : GeneralOptions
        An instance of GeneralOptions class containing all the general options.

    Returns
    -------
    None
    """
    iq.phylogenetic_analysis(aln_file, partition_file, tree_file, out_prefix, num_threads, seed, general_opts)

@command(cli, "parsimony-multistate")
@option("--input-file", type=str, help="Path to the input file containing multistate data")
@general_options
def parsimony_multistate(input_file: str, general_opts: GeneralOptions):
    """
    Run parsimony multistate analysis using specified parameters.

    Parameters
    ----------
    input_file : str
        Path to the input file containing multistate data.
    general_opts : GeneralOptions
        An instance of GeneralOptions class containing all the general options.

    Returns
    -------
    None
    """
    iq.parsimony_multistate(input_file, general_opts)

@command(cli, "rf-distance")
@option("--tree1-file", type=str, help="Path to the first tree file")
@option("--tree2-file", type=str, help="Path to the second tree file")
@general_options
def rf_distance(tree1_file: str, tree2_file: str, general_opts: GeneralOptions):
    """
    Compute RF distance using specified parameters.

    Parameters
    ----------
    tree1_file : str
        Path to the first tree file.
    tree2_file : str
        Path to the second tree file.
    general_opts : GeneralOptions
        An instance of GeneralOptions class containing all the general options.

    Returns
    -------
    None
    """
    iq.rf_distance(tree1_file, tree2_file, general_opts)

if __name__ == '__main__':
    cli()


if __name__ == '__main__':
    cli()
