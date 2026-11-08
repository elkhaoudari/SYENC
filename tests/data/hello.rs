use std::collections::HashMap;

fn main() {
    let mut counts: HashMap<char, usize> = HashMap::new();
    for c in "hello world".chars() {
        *counts.entry(c).or_insert(0) += 1;
    }
    println!("{:?}", counts.get(&'l'));
}
