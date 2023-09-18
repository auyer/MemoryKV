use serde::Deserialize;

#[derive(Deserialize, Debug)]
pub struct Configuration {
    #[serde(default = "default_port")]
    pub port: u16,
    #[serde(default = "default_metrics_port")]
    pub metrics_port: u16,
    #[serde(default = "default_as_false")]
    pub debug: bool,
    #[serde(default = "default_as_false")]
    pub trace: bool,
}

fn default_as_false() -> bool {
    false
}
fn default_port() -> u16 {
    8080
}
fn default_metrics_port() -> u16 {
    3001
}

pub fn read_configuration() -> Configuration {
    let config =
        envy::from_env::<Configuration>().expect("Please provide PORT and METRICS_PORT env var");

    println!("{:#?}", config);
    config
}
