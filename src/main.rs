use clap::Parser;
use seminar::benchmarks;

#[derive(Parser, Debug)]
#[command(version, about = "Parallel benchmark suite")]
struct Args {
    /// Benchmark to run: histogram | mergesort | stencil | bfs
    #[arg(short, long)]
    benchmark: String,

    /// Input specifier (e.g. "1000_uniform_256", "1000_u32_cutoff256", "100_k10_r1", or graph path)
    #[arg(short, long)]
    input: String,

    /// Implementation: rust_seq | rust_rayon | cpp_openmp | cpp_taskflow
    #[arg(short = 'm', long = "impl")]
    implementation: String,

    /// Number of threads
    #[arg(short, long, default_value_t = 1)]
    threads: usize,

    /// Run identifier (optional, defaults to 1)
    #[arg(short, long, default_value_t = 1)]
    run: usize,
}

fn main() {
    let args = Args::parse();

    match args.benchmark.as_str() {
        "histogram" => match args.implementation.as_str() {
            "rust_seq" => {
                benchmarks::histogram::sequential::run(&args.input, args.threads, args.run)
            }
            "rust_rayon" => benchmarks::histogram::rayon::run(&args.input, args.threads, args.run),
            _ => {
                eprintln!("unknown implementation: {}", args.implementation);
                std::process::exit(1);
            }
        },
        "mergesort" => match args.implementation.as_str() {
            "rust_seq" => {
                benchmarks::mergesort::sequential::run(&args.input, args.threads, args.run)
            }
            "rust_rayon" => benchmarks::mergesort::rayon::run(&args.input, args.threads, args.run),
            _ => {
                eprintln!("unknown implementation: {}", args.implementation);
                std::process::exit(1);
            }
        },
        "stencil" => match args.implementation.as_str() {
            "rust_seq" => benchmarks::stencil::sequential::run(&args.input, args.threads, args.run),
            "rust_rayon" => benchmarks::stencil::rayon::run(&args.input, args.threads, args.run),
            _ => {
                eprintln!("unknown implementation: {}", args.implementation);
                std::process::exit(1);
            }
        },
        "bfs" => match args.implementation.as_str() {
            "rust_seq" => benchmarks::bfs::sequential::run(&args.input, args.threads, args.run),
            "rust_rayon" => benchmarks::bfs::rayon::run(&args.input, args.threads, args.run),
            _ => {
                eprintln!("unknown implementation: {}", args.implementation);
                std::process::exit(1);
            }
        },
        _ => {
            eprintln!("unknown benchmark: {}", args.benchmark);
            std::process::exit(1);
        }
    }
}
