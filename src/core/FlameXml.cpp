#include "FlameXml.hpp"
#include <QFile>
#include <QFileInfo>
#include <QStringConverter>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <sstream>

namespace ApoNeo::Core {

namespace {

void parse_xform_element(QXmlStreamReader& reader, Xform& xf) {
    const auto attrs = reader.attributes();

    if (attrs.hasAttribute("weight")) {
        xf.weight = attrs.value("weight").toDouble();
    }
    if (attrs.hasAttribute("color")) {
        xf.color_index = attrs.value("color").toDouble();
    }
    if (attrs.hasAttribute("color_speed")) {
        xf.color_speed = attrs.value("color_speed").toDouble();
    }
    if (attrs.hasAttribute("opacity")) {
        xf.opacity = attrs.value("opacity").toDouble();
    }
    if (attrs.hasAttribute("name")) {
        xf.name = attrs.value("name").toString().toStdString();
    }

    // Affine transform: coefs="a d b e c f" or "a b c d e f" (standard flam3 uses a d b e c f for 2x3 matrix)
    if (attrs.hasAttribute("coefs")) {
        QStringList parts = attrs.value("coefs").toString().split(u' ', Qt::SkipEmptyParts);
        if (parts.size() >= 6) {
            xf.affine.a = parts[0].toDouble();
            xf.affine.d = parts[1].toDouble();
            xf.affine.b = parts[2].toDouble();
            xf.affine.e = parts[3].toDouble();
            xf.affine.c = parts[4].toDouble();
            xf.affine.f = parts[5].toDouble();
        }
    }

    // Post-affine transform
    if (attrs.hasAttribute("post")) {
        QStringList parts = attrs.value("post").toString().split(u' ', Qt::SkipEmptyParts);
        if (parts.size() >= 6) {
            xf.post_affine.a = parts[0].toDouble();
            xf.post_affine.d = parts[1].toDouble();
            xf.post_affine.b = parts[2].toDouble();
            xf.post_affine.e = parts[3].toDouble();
            xf.post_affine.c = parts[4].toDouble();
            xf.post_affine.f = parts[5].toDouble();
            xf.has_post_affine = true;
        }
    }

    // Variations & parameters from attributes
    auto& reg = VariationRegistry::instance();
    for (const auto& attr : attrs) {
        std::string attr_name = attr.name().toString().toStdString();
        double val = attr.value().toDouble();

        if (reg.find_by_name(attr_name)) {
            xf.set_variation(attr_name, val);
        } else if (attr_name.find('_') != std::string::npos) {
            // Variation parameter like waves_b, popcorn_c
            xf.set_param(attr_name, val);
        }
    }

    // Chaos weights
    if (attrs.hasAttribute("chaos")) {
        QStringList parts = attrs.value("chaos").toString().split(u' ', Qt::SkipEmptyParts);
        xf.chaos_weights.clear();
        for (const auto& p : parts) {
            xf.chaos_weights.push_back(p.toDouble());
        }
    }

    xf.compile_variations();
}

bool parse_flame_node(QXmlStreamReader& reader, FlameGenome& g) {
    const auto attrs = reader.attributes();

    if (attrs.hasAttribute("name")) {
        g.name = attrs.value("name").toString().toStdString();
    }
    if (attrs.hasAttribute("size")) {
        QStringList parts = attrs.value("size").toString().split(u' ', Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            g.width = parts[0].toInt();
            g.height = parts[1].toInt();
        }
    }
    if (attrs.hasAttribute("scale")) {
        g.scale = attrs.value("scale").toDouble();
    }
    if (attrs.hasAttribute("center")) {
        QStringList parts = attrs.value("center").toString().split(u' ', Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            g.center_x = parts[0].toDouble();
            g.center_y = parts[1].toDouble();
        }
    }
    if (attrs.hasAttribute("zoom")) {
        g.zoom = attrs.value("zoom").toDouble();
    }
    if (attrs.hasAttribute("rotate")) {
        g.rotate = attrs.value("rotate").toDouble();
    }
    if (attrs.hasAttribute("oversample") || attrs.hasAttribute("supersample")) {
        g.supersample = attrs.hasAttribute("oversample") ? attrs.value("oversample").toInt() : attrs.value("supersample").toInt();
    }
    if (attrs.hasAttribute("quality")) {
        g.quality = attrs.value("quality").toDouble();
    }
    if (attrs.hasAttribute("gamma")) {
        g.tone_map.gamma = attrs.value("gamma").toDouble();
    }
    if (attrs.hasAttribute("brightness")) {
        g.tone_map.brightness = attrs.value("brightness").toDouble();
    }
    if (attrs.hasAttribute("vibrancy")) {
        g.tone_map.vibrancy = attrs.value("vibrancy").toDouble();
    }
    if (attrs.hasAttribute("background")) {
        QStringList parts = attrs.value("background").toString().split(u' ', Qt::SkipEmptyParts);
        if (parts.size() >= 3) {
            g.background = ColorRGBA(parts[0].toFloat(), parts[1].toFloat(), parts[2].toFloat(), 1.0f);
        }
    }

    g.xforms.clear();
    g.has_final_xform = false;

    // Read child elements (<xform>, <finalxform>, <palette>, <colors>)
    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::EndElement && reader.name() == u"flame") {
            break;
        }

        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == u"xform") {
                Xform xf;
                parse_xform_element(reader, xf);
                g.xforms.push_back(std::move(xf));
            } else if (reader.name() == u"finalxform") {
                Xform fxf;
                parse_xform_element(reader, fxf);
                g.final_xform = std::move(fxf);
                g.has_final_xform = true;
            } else if (reader.name() == u"palette" || reader.name() == u"colors") {
                QString hex_text = reader.readElementText();
                g.palette.from_hex_string(hex_text.toStdString());
            } else {
                reader.skipCurrentElement();
            }
        }
    }

    g.compile();
    return true;
}

} // namespace

std::vector<FlameGenome> FlameXml::parse_flames(const std::string& xml_content, std::string* error_out) {
    std::vector<FlameGenome> results;
    QXmlStreamReader reader(QString::fromStdString(xml_content));

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == u"flame") {
                FlameGenome g;
                if (parse_flame_node(reader, g)) {
                    results.push_back(std::move(g));
                }
            }
        }
    }

    if (reader.hasError() && error_out) {
        *error_out = reader.errorString().toStdString();
    }

    return results;
}

bool FlameXml::parse_single_flame(const std::string& xml_content, FlameGenome& out_genome, std::string* error_out) {
    auto flames = parse_flames(xml_content, error_out);
    if (flames.empty()) {
        if (error_out && error_out->empty()) {
            *error_out = "No <flame> elements found in XML.";
        }
        return false;
    }
    out_genome = std::move(flames.front());
    return true;
}

std::vector<FlameGenome> FlameXml::load_from_file(const std::string& filepath, std::string* error_out) {
    QFile file(QString::fromStdString(filepath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error_out) {
            *error_out = "Unable to open file: " + filepath;
        }
        return {};
    }

    QByteArray data = file.readAll();
    return parse_flames(data.toStdString(), error_out);
}

static void write_xform_xml(QXmlStreamWriter& writer, const Xform& xf, bool is_final) {
    writer.writeStartElement(is_final ? QStringLiteral("finalxform") : QStringLiteral("xform"));
    if (!is_final) {
        writer.writeAttribute(QStringLiteral("weight"), QString::number(xf.weight, 'g', 6));
    }
    writer.writeAttribute(QStringLiteral("color"), QString::number(xf.color_index, 'g', 6));
    if (std::abs(xf.color_speed - 0.5) > 1e-4) {
        writer.writeAttribute(QStringLiteral("color_speed"), QString::number(xf.color_speed, 'g', 6));
    }
    if (std::abs(xf.opacity - 1.0) > 1e-4) {
        writer.writeAttribute(QStringLiteral("opacity"), QString::number(xf.opacity, 'g', 6));
    }
    if (!xf.name.empty()) {
        writer.writeAttribute(QStringLiteral("name"), QString::fromStdString(xf.name));
    }

    // Coefs: a d b e c f
    QString coefs = QString("%1 %2 %3 %4 %5 %6")
        .arg(xf.affine.a, 0, 'g', 6)
        .arg(xf.affine.d, 0, 'g', 6)
        .arg(xf.affine.b, 0, 'g', 6)
        .arg(xf.affine.e, 0, 'g', 6)
        .arg(xf.affine.c, 0, 'g', 6)
        .arg(xf.affine.f, 0, 'g', 6);
    writer.writeAttribute(QStringLiteral("coefs"), coefs);

    if (xf.has_post_affine) {
        QString post = QString("%1 %2 %3 %4 %5 %6")
            .arg(xf.post_affine.a, 0, 'g', 6)
            .arg(xf.post_affine.d, 0, 'g', 6)
            .arg(xf.post_affine.b, 0, 'g', 6)
            .arg(xf.post_affine.e, 0, 'g', 6)
            .arg(xf.post_affine.c, 0, 'g', 6)
            .arg(xf.post_affine.f, 0, 'g', 6);
        writer.writeAttribute(QStringLiteral("post"), post);
    }

    for (const auto& [var_name, var_weight] : xf.variation_weights) {
        writer.writeAttribute(QString::fromStdString(var_name), QString::number(var_weight, 'g', 6));
    }

    for (const auto& [param_name, param_val] : xf.params) {
        writer.writeAttribute(QString::fromStdString(param_name), QString::number(param_val, 'g', 6));
    }

    if (!xf.chaos_weights.empty()) {
        QStringList chaos_strs;
        for (double cw : xf.chaos_weights) {
            chaos_strs.append(QString::number(cw, 'g', 6));
        }
        writer.writeAttribute(QStringLiteral("chaos"), chaos_strs.join(u' '));
    }

    writer.writeEndElement(); // xform / finalxform
}

static void write_flame_xml(QXmlStreamWriter& writer, const FlameGenome& g) {
    writer.writeStartElement(QStringLiteral("flame"));
    writer.writeAttribute(QStringLiteral("name"), QString::fromStdString(g.name));
    writer.writeAttribute(QStringLiteral("version"), QStringLiteral("ApoNeo 1.0"));
    writer.writeAttribute(QStringLiteral("size"), QString("%1 %2").arg(g.width).arg(g.height));
    writer.writeAttribute(QStringLiteral("center"), QString("%1 %2").arg(g.center_x, 0, 'g', 6).arg(g.center_y, 0, 'g', 6));
    writer.writeAttribute(QStringLiteral("scale"), QString::number(g.scale, 'g', 6));
    writer.writeAttribute(QStringLiteral("rotate"), QString::number(g.rotate, 'g', 6));
    writer.writeAttribute(QStringLiteral("zoom"), QString::number(g.zoom, 'g', 6));
    writer.writeAttribute(QStringLiteral("oversample"), QString::number(g.supersample));
    writer.writeAttribute(QStringLiteral("quality"), QString::number(g.quality, 'g', 6));
    writer.writeAttribute(QStringLiteral("gamma"), QString::number(g.tone_map.gamma, 'g', 4));
    writer.writeAttribute(QStringLiteral("brightness"), QString::number(g.tone_map.brightness, 'g', 4));
    writer.writeAttribute(QStringLiteral("vibrancy"), QString::number(g.tone_map.vibrancy, 'g', 4));
    writer.writeAttribute(QStringLiteral("background"), QString("%1 %2 %3").arg(g.background.r, 0, 'g', 4).arg(g.background.g, 0, 'g', 4).arg(g.background.b, 0, 'g', 4));

    for (const auto& xf : g.xforms) {
        write_xform_xml(writer, xf, false);
    }

    if (g.has_final_xform) {
        write_xform_xml(writer, g.final_xform, true);
    }

    writer.writeStartElement(QStringLiteral("palette"));
    writer.writeAttribute(QStringLiteral("count"), QStringLiteral("256"));
    writer.writeAttribute(QStringLiteral("format"), QStringLiteral("RGB"));
    writer.writeCharacters(QString::fromStdString(g.palette.to_hex_string()));
    writer.writeEndElement(); // palette

    writer.writeEndElement(); // flame
}

std::string FlameXml::serialize_flame(const FlameGenome& genome) {
    return serialize_flames({genome});
}

std::string FlameXml::serialize_flames(const std::vector<FlameGenome>& genomes) {
    QString output;
    QXmlStreamWriter writer(&output);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(2);
    writer.writeStartDocument();

    writer.writeStartElement(QStringLiteral("flames"));
    writer.writeAttribute(QStringLiteral("name"), QStringLiteral("flames"));

    for (const auto& g : genomes) {
        write_flame_xml(writer, g);
    }

    writer.writeEndElement(); // flames
    writer.writeEndDocument();

    return output.toStdString();
}

bool FlameXml::save_to_file(const std::string& filepath, const FlameGenome& genome, std::string* error_out) {
    return save_flames_to_file(filepath, {genome}, error_out);
}

bool FlameXml::save_flames_to_file(const std::string& filepath, const std::vector<FlameGenome>& genomes, std::string* error_out) {
    QFile file(QString::fromStdString(filepath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error_out) {
            *error_out = "Unable to write file: " + filepath;
        }
        return false;
    }

    std::string xml = serialize_flames(genomes);
    file.write(xml.data(), static_cast<qint64>(xml.size()));
    return true;
}

} // namespace ApoNeo::Core
